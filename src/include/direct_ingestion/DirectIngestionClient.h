#pragma once

#include <queue>
#include "common/WavefrontSender.h"
#include "direct_ingestion/DirectIngesterService.h"

namespace wavefront {
    class DirectIngestionClient : public WavefrontSender {
    public:
        // nested class for client builder
        struct Builder {
            Builder(const std::string &serverName, const std::string &token) : serverName(serverName), token(token) {
            }

            Builder setMaxQueueSize(int maxQueueSize) {
                this->maxQueueSize = maxQueueSize;
                return *this;
            }

            Builder setFlushingInterval(int flushIntervalSeconds) {
                this->flushIntervalSeconds = flushIntervalSeconds;
                return *this;
            }

            Builder setBatchSize(int batchSize) {
                this->batchSize = batchSize;
                return *this;
            }

            DirectIngestionClient *build() {
                return new DirectIngestionClient(this);
            }

            // Required parameters
            std::string serverName;
            std::string token;

            // Optional parameters
            int maxQueueSize = 10;
            int batchSize = 10000;
            int flushIntervalSeconds = 5;
        };

        void sendMetric(const std::string &name, double value, long timestamp = -1, const std::string &source = "",
                        std::map<std::string, std::string> tags = {{}}) override;

        void sendDistribution(const std::string &name, std::list<std::pair<double, int>> centroids,
                              std::set<HistogramGranularity> histogramGranularities, long timestamp = -1,
                              const std::string &source = "",
                              std::map<std::string, std::string> tags = {{}}) override;

        void sendSpan(const std::string &name, long startMillis, long durationMillis,
                      boost::uuids::uuid traceId, boost::uuids::uuid spanId, const std::string &source = "",
                      std::list<boost::uuids::uuid> parents = {}, std::list<boost::uuids::uuid> followsFrom = {},
                      std::map<std::string, std::string> tags = {{}}) override;

        int getFailureCount();

        void close();

        /**
         * start a seperate thread to flush queued content every @flushIntervalSecondsa
         * MUST be called after DirectIngestionClient is constructed
         */
        void start();

    private:
        DirectIngestionClient(Builder *builder);

        void flushTask();

        void flush();

        void internalFlush(std::queue<std::string> &buffer, const std::string &format);

        // source is hardcoded
        std::string defaultSource = "wavefrontDirectSender";
        int batchSize;
        int maxQueueSize;
        int flushIntervalSeconds;

        std::mutex mutex;
        std::queue<std::string> metricsBuffer;
        std::queue<std::string> histogramBuffer;
        std::queue<std::string> tracingBuffer;
        std::atomic<int> failures;

        DirectIngesterService service;
        // thread dedicated for flushing task
        std::thread t;
    };
}