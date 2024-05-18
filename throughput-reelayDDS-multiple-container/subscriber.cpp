/*
 * Copyright(c) 2022 ZettaScale Technology and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */

#include <map>
#include <iomanip>
#include <iostream>
#include <csignal>
#include <thread>
#include <string>
#include "dds/dds.hpp"
#include "Throughput.hpp"
// Include headers for Reelay, Boost, and simdjson
#include "reelay/monitors.hpp"
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <cstddef>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <atomic>
#include "simdjson.h"

/*
 * The Throughput example measures data throughput in bytes per second. The publisher
 * allows you to specify a payload size in bytes as well as allowing you to specify
 * whether to send data in bursts. The publisher will continue to send data forever
 * unless a time out is specified. The subscriber will receive data and output the
 * total amount received and the data rate in bytes per second. It will also indicate
 * if any samples were received out of order. A maximum number of cycles can be
 * specified and once this has been reached the subscriber will terminate and output
 * totals and averages.
 */

namespace reelay {

template<typename T>
struct timefield<T, simdjson::dom::element> {
  using input_t = simdjson::dom::element;
  inline static T get_time(const input_t& container)
  {
    return container.at_key("time");
  }
};

template<>
struct datafield<simdjson::dom::element> {
  using input_t = simdjson::dom::element;

  inline static input_t at(const input_t& container, const std::string& key)
  {
    return container.at_key(key);
  }

  inline static input_t at(const input_t& container, std::size_t index)
  {
    return container.at(index);
  }

  inline static bool contains(const input_t& container, const std::string& key)
  {
    for(simdjson::dom::object::iterator field =
          simdjson::dom::object(container).begin();
        field != simdjson::dom::object(container).end();
        ++field) {
      if(key == field.key()) {
        return true;
      }
    }
    return false;
  }

  inline static bool as_bool(const input_t& container, const std::string& key)
  {
    return container.at_key(key).get<bool>();
  }

  inline static int64_t as_integer(
    const input_t& container, const std::string& key)
  {
    return container.at_key(key).get<int64_t>();
  }

  inline static double as_floating(
    const input_t& container, const std::string& key)
  {
    return container.at_key(key).get<double>();
  }

  inline static std::string as_string(
    const input_t& container, const std::string& key)
  {
    std::string_view sv = container.at_key(key).get<std::string_view>();
    return std::string(sv);
  }

  inline static bool contains(const input_t& container, std::size_t index)
  {
    return index < simdjson::dom::array(container).size();
  }

  inline static bool as_bool(const input_t& container, std::size_t index)
  {
    return container.at(index).get<bool>();
  }

  inline static int as_integer(const input_t& container, std::size_t index)
  {
    return container.at(index).get<int64_t>();
  }

  inline static double as_floating(const input_t& container, std::size_t index)
  {
    return container.at(index).get<double>();
  }

  inline static std::string as_string(
    const input_t& container, std::size_t index)
  {
    std::string_view sv = container.at(index).get<std::string_view>();
    return std::string(sv);
  }
};
} // namespace reelay

// Reelay, Boost, and simdjson namespaces
namespace ry = reelay;
namespace sj = simdjson;
namespace po = boost::program_options;

using input_t = simdjson::dom::element;
using output_t = reelay::json;

std::atomic<bool> keep_running(true); // for automatic callback exit //mhn4d --> do we need it in this code ????

simdjson::dom::parser parser;
simdjson::dom::element json_element;

// Timing variables to measure the duration of message processing
std::chrono::high_resolution_clock::time_point first_received;
std::chrono::high_resolution_clock::time_point last_received;

bool first_message = true; // if the first message has been received
int message_count = 0; // message count for throughput calculation

//====================================================================
#define BYTES_PER_SEC_TO_MEGABITS_PER_SEC 125000
#define MAX_SAMPLES 1000
#define subprefix "=== [Subscriber] "

using namespace org::eclipse::cyclonedds;

static unsigned long long outOfOrder(0); /*keeps track of out of order samples*/

static unsigned long long total_bytes(0); /*keeps track of total bytes received*/

static unsigned long payloadSize(0); /*size of the last payload received*/

static unsigned long long total_samples(0); /*keeps track of total samples received*/

static std::chrono::milliseconds pollingDelay(-1); /*default is a listener*/

static unsigned long long maxCycles(0); /*maximum number of display cycles to show*/

static std::string partitionName("Throughput example"); /*name of the domain on which the throughput test is run*/

static std::map<dds::core::InstanceHandle, unsigned long long> mp; /*collection of expected sequence numbers*/

static volatile sig_atomic_t done(false); /*semaphore for keeping track of whether to run the test*/

static std::string reelay_expr("{p}"); /* The default Reelay expression */

static int parse_args(int argc, char **argv)
{
  /*
   * Get the program parameters
   * Parameters: subscriber [maxCycles] [pollingDelay] [partitionName]
   */
  if (argc == 2 && (strcmp (argv[1], "-h") == 0 || strcmp (argv[1], "--help") == 0))
  {
    std::cout << subprefix << "Usage (parameters must be supplied in order):\n" <<
                 subprefix << "./subscriber [maxCycles (0 = infinite)] [pollingDelay (ms, 0 = no polling, use waitset, -1 = no polling, use listener)] [partitionName] [reelayExpression ({p})]\n" <<
                 subprefix << "Defaults:\n" <<
                 subprefix << "./subscriber 0 0 \"Throughput example\"\n" << std::flush;
    return EXIT_FAILURE;
  }

  if (argc > 1)
  {
    maxCycles = static_cast<unsigned long long>(atoi (argv[1])); /* The number of times to output statistics before terminating */
  }
  if (argc > 2)
  {
    pollingDelay = std::chrono::milliseconds(atoi(argv[2])); /* The number of ms to wait between reads (0 = waitset, -1 = listener) */
  }
  if (argc > 3)
  {
    partitionName = argv[3]; /* The name of the partition */
  }
  if (argc > 4)
  {
      reelay_expr = argv[4]; /* The Reelay expression */
  }
  return EXIT_SUCCESS;
}

void monitor_on_sample(const ThroughputModule::DataType& data, void* context) {
    last_received = std::chrono::high_resolution_clock::now(); // Update last received time

    auto* monitor = (reelay::monitor<input_t, output_t>*) context;
    auto json_string = simdjson::padded_string(reinterpret_cast<const char*>(data.payload().data()), data.payload().size());
    json_element = parser.parse(json_string.data(), json_string.size());
    auto result = monitor->update(json_element); //mhn4d --> this line takes 4 seconds to execute ??????????????? and there is no diff with zenoh project
    // if (!result.empty()) {
    //     std::cout << result << std::endl;
    // }

    if (first_message) {
        first_received = std::chrono::high_resolution_clock::now();
        first_message = false;
    }

    message_count++;
}




unsigned long long do_take(dds::sub::DataReader<ThroughputModule::DataType>& rd)
{
  
  // last_received = std::chrono::high_resolution_clock::now(); // Update last received time
  auto samples = rd.take();
  unsigned long long valid_samples = 0;
  for (const auto & s:samples)
  {
    if (!s.info().valid())
      continue;

    auto pub_handle = s.info().publication_handle();
    auto ct = s.data().count();
    auto it = mp.insert({pub_handle,ct}).first;

    /*check whether the received sequence number matches that which we expect*/
    if (it->second != ct)
      outOfOrder++;

    valid_samples++;
    payloadSize = static_cast<unsigned long>(s.data().payload().size());
    total_bytes += payloadSize;
    it->second = ct+1;

    // //Print the received payload
    // std::string received_payload(s.data().payload().begin(), s.data().payload().end());
    // // std::cout << subprefix << "Received payload: " << received_payload << std::endl;

    // // Reelay monitoring
    // json_element = parser.parse(received_payload);
    // auto result = monitor.update(json_element);
    // // if (!result.empty()) {
    // //   std::cout << result << std::endl;
    // // }

    // if (first_message) {
    //   first_received = std::chrono::high_resolution_clock::now();
    //   first_message = false;
    // }

    // message_count++;
  }

  total_samples += valid_samples;
  return valid_samples;
}

void process_samples(dds::sub::DataReader<ThroughputModule::DataType> &reader, size_t max_c)
{
  unsigned long long prev_bytes = 0;
  unsigned long long prev_samples = 0;
  unsigned long cycles = 0;
  auto startTime = std::chrono::steady_clock::now();
  auto prev_time = startTime;

  dds::core::cond::WaitSet waitset;

  dds::core::cond::StatusCondition sc = dds::core::cond::StatusCondition(reader);
  sc.enabled_statuses(dds::core::status::StatusMask::data_available());
  waitset.attach_condition(sc);

  dds::core::Duration waittime =
    dds::core::Duration::from_millisecs(pollingDelay.count() > 0 ? pollingDelay.count() : 100);

  while (!done && (max_c == 0 || cycles < max_c))
  {
    dds::core::cond::WaitSet::ConditionSeq conditions;
    bool wait_again = true;
    try {
      if (pollingDelay.count() == 0) {
        conditions = waitset.wait(waittime);
      } else {
        wait_again = false;
        std::this_thread::sleep_for(pollingDelay.count() > 0 ? pollingDelay : std::chrono::milliseconds(100));
      }
    } catch (const dds::core::TimeoutError &) {
      done = true;
    } catch (const dds::core::Exception &e) {
      std::cout << subprefix << "Waitset encountered the following error: \"" << e.what() << "\".\n" << std::flush;
      break;
    } catch (...) {
      std::cout << subprefix << "Waitset encountered an unknown error.\n" << std::flush;
      break;
    }

    if (reader.subscription_matched_status().current_count() == 0)
      done = true;

    for (const auto &c:conditions)
    {
      if (c.trigger_value())
      {
        wait_again = false;
        break;
      }
    }

    if (wait_again)
      continue;

    /*we only do take in the case of a waitset/polling approach,
      as the listener will have already processed the incoming messages*/
    if (pollingDelay.count() >= 0)
    {
      while (do_take (reader))
        ;
    }

    auto time_now = std::chrono::steady_clock::now();

    if (prev_time == startTime)
    {
      prev_time = time_now;
    } else
    {
      if (time_now > prev_time + std::chrono::seconds(1) && total_samples != prev_samples)
      {
        /* Output intermediate statistics */
        auto deltaTime = std::chrono::duration<double>(time_now - prev_time).count();
        printf ("=== [Subscriber] %5.3f Payload size: %lu | Total received: %llu samples, %llu bytes | Out of order: %llu samples "
                "Transfer rate: %.2lf samples/s, %.2lf Mbit/s\n",
                deltaTime, payloadSize, total_samples, total_bytes, outOfOrder,
                (deltaTime != 0.0) ? (static_cast<double>(total_samples - prev_samples) / deltaTime) : 0,
                (deltaTime != 0.0) ? ((static_cast<double>(total_bytes - prev_bytes) / BYTES_PER_SEC_TO_MEGABITS_PER_SEC) / deltaTime) : 0);
        fflush (stdout);
        cycles++;
        prev_time = time_now;
        prev_bytes = total_bytes;
        prev_samples = total_samples;
      }
    }
  }
  /* Output totals and averages */
  auto deltaTime = std::chrono::duration<double>(std::chrono::steady_clock::now() - startTime).count();
  printf ("\nTotal received: %llu samples, %llu bytes\n", total_samples, total_bytes);
  printf ("Out of order: %llu samples\n", outOfOrder);
  printf ("Average transfer rate: %.2lf samples/s, ", static_cast<double>(total_samples) / deltaTime);
  printf ("%.2lf Mbit/s\n", (static_cast<double>(total_bytes) / BYTES_PER_SEC_TO_MEGABITS_PER_SEC) / deltaTime);
  fflush (stdout);
  
  // Output total time and throughput of monitoring
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(last_received - first_received).count();
  double seconds = duration / 1000.0; // milliseconds to seconds
  double throughput = message_count / seconds; // throughput as messages per second

  std::cout << "Total Time: " << seconds << "s, Total Messages: " << message_count << ", Throughput: " << throughput << " msgs/s" << std::endl;
}

static void sigint (int sig)
{
  (void) sig;
  done = true;
}

bool wait_for_writer(dds::sub::DataReader<ThroughputModule::DataType> &reader)
{
  std::cout << "\n" << subprefix << "Waiting for a writer ...\n" << std::flush;

  dds::core::cond::StatusCondition sc = dds::core::cond::StatusCondition(reader);
  sc.enabled_statuses(dds::core::status::StatusMask::subscription_matched());

  dds::core::cond::WaitSet waitset;
  waitset.attach_condition(sc);

  dds::core::cond::WaitSet::ConditionSeq conditions =
    waitset.wait(dds::core::Duration::from_secs(30));

  for (const auto & c:conditions)
  {
    if (c == sc)
      return true;
  }

  std::cout << subprefix << "Did not discover a writer.\n" << std::flush;

  return false;
}

class ThroughputListener : public dds::sub::NoOpDataReaderListener<ThroughputModule::DataType> {
public:
    ThroughputListener(reelay::monitor<input_t, output_t>& monitor) : monitor(monitor) {}

    void on_data_available(dds::sub::DataReader<ThroughputModule::DataType>& reader) override {
        auto samples = reader.take();
        for (const auto& s : samples) {
            if (s.info().valid()) {
                monitor_on_sample(s.data(), &monitor);
            }
        }
    }

private:
    reelay::monitor<input_t, output_t>& monitor;
};




int main(int argc, char** argv) {
    try {
        if (parse_args(argc, argv) == EXIT_FAILURE) {
            return EXIT_FAILURE;
        }

        std::cout << subprefix << "Cycles: " << maxCycles << " | PollingDelay: " << pollingDelay.count() << " ms | Partition: " << partitionName << "\n"
                  << subprefix << "Using a " << (pollingDelay.count() > 0 ? "polling" : pollingDelay.count() < 0 ? "listener" : "waitset") << " approach.\n" << std::flush;

        // Reelay monitor construction
        using input_t = simdjson::dom::element;
        using output_t = reelay::json;
        auto manager = std::make_shared<reelay::binding_manager>();
        auto monitor = reelay::monitor<input_t, output_t>();

        auto monitor_opts = reelay::discrete_timed<int64_t>::monitor<input_t, output_t>::options()
            .with_condensing(true)
            .with_data_manager(manager);

        monitor = reelay::make_monitor(reelay_expr, monitor_opts);

        dds::domain::DomainParticipant participant(domain::default_id());

        dds::topic::qos::TopicQos tqos;
        tqos << dds::core::policy::Reliability::Reliable(dds::core::Duration::from_secs(10))
             << dds::core::policy::History::KeepAll()
             << dds::core::policy::ResourceLimits(MAX_SAMPLES);

        dds::topic::Topic<ThroughputModule::DataType> topic(participant, "bounverif/timescales", tqos);

        dds::sub::qos::SubscriberQos sqos;
        sqos << dds::core::policy::Partition(partitionName);

        dds::sub::Subscriber subscriber(participant, sqos);

        // Pass the monitor by reference to the ThroughputListener
        ThroughputListener listener(monitor);

        dds::sub::DataReader<ThroughputModule::DataType> reader(
            subscriber,
            topic,
            dds::sub::qos::DataReaderQos(tqos),
            pollingDelay.count() < 0 ? &listener : NULL,
            pollingDelay.count() < 0 ? dds::core::status::StatusMask::data_available() : dds::core::status::StatusMask::none());

        signal(SIGINT, sigint);

        if (wait_for_writer(reader)) {
            std::cout << subprefix << "Waiting for samples...\n" << std::flush;
            process_samples(reader, maxCycles);
        }
    } catch (const dds::core::Exception& e) {
        std::cerr << "DDS exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    } catch (const std::exception& e) {
        std::cerr << "C++ exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "Generic exception" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}



