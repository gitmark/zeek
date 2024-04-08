// See the file "COPYING" in the main distribution directory for copyright.

#pragma once

#include <condition_variable>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <string_view>
#include <vector>

#include "zeek/IntrusivePtr.h"
#include "zeek/Span.h"
#include "zeek/telemetry/Counter.h"
#include "zeek/telemetry/Gauge.h"
#include "zeek/telemetry/Histogram.h"
#include "zeek/telemetry/ProcessStats.h"

#include "prometheus/exposer.h"
#include "prometheus/registry.h"

namespace zeek {
class RecordVal;
using RecordValPtr = IntrusivePtr<RecordVal>;
} // namespace zeek

namespace zeek::telemetry {

class OtelReader;

/**
 * Manages a collection of metric families.
 */
class Manager final {
public:
    Manager();

    Manager(const Manager&) = delete;

    Manager& operator=(const Manager&) = delete;

    ~Manager() = default;

    /**
     * Initialization of the manager. This is called late during Zeek's
     * initialization after any scripts are processed.
     */
    void InitPostScript();

    /**
     * @return A VectorVal containing all counter and gauge metrics and their values matching prefix and name.
     * @param prefix The prefix pattern to use for filtering. Supports globbing.
     * @param name The name pattern to use for filtering. Supports globbing.
     */
    ValPtr CollectMetrics(std::string_view prefix, std::string_view name);

    /**
     * @return A VectorVal containing all histogram metrics and their values matching prefix and name.
     * @param prefix The prefix pattern to use for filtering. Supports globbing.
     * @param name The name pattern to use for filtering. Supports globbing.
     */
    ValPtr CollectHistogramMetrics(std::string_view prefix, std::string_view name);

    /**
     * @return A counter metric family. Creates the family lazily if necessary.
     * @param prefix The prefix (namespace) this family belongs to.
     * @param name The human-readable name of the metric, e.g., `requests`.
     * @param labels Names for all label dimensions of the metric.
     * @param helptext Short explanation of the metric.
     * @param unit Unit of measurement.
     * @param is_sum Indicates whether this metric accumulates something, where only the total value is of interest.
     * @param callback Passing a callback method will enable asynchronous mode. The callback method will be called by
     * the metrics subsystem whenever data is requested.
     */
    template<class ValueType = int64_t>
    auto CounterFamily(std::string_view prefix, std::string_view name, Span<const std::string_view> labels,
                       std::string_view helptext, std::string_view unit = "", bool is_sum = false) {
        auto fam = LookupFamily(prefix, name);

        if constexpr ( std::is_same<ValueType, int64_t>::value ) {
            if ( fam )
                return std::static_pointer_cast<IntCounterFamily>(fam);

            auto int_fam =
                std::make_shared<IntCounterFamily>(prefix, name, labels, helptext, prometheus_registry, unit, is_sum);
            families.insert_or_assign(int_fam->FullName(), int_fam);
            return int_fam;
        }
        else {
            static_assert(std::is_same<ValueType, double>::value, "metrics only support int64_t and double values");

            if ( fam )
                return std::static_pointer_cast<DblCounterFamily>(fam);

            auto dbl_fam =
                std::make_shared<DblCounterFamily>(prefix, name, labels, helptext, prometheus_registry, unit, is_sum);
            families.insert_or_assign(dbl_fam->FullName(), dbl_fam);
            return dbl_fam;
        }
    }

    /// @copydoc CounterFamily
    template<class ValueType = int64_t>
    auto CounterFamily(std::string_view prefix, std::string_view name, std::initializer_list<std::string_view> labels,
                       std::string_view helptext, std::string_view unit = "", bool is_sum = false) {
        auto lbl_span = Span{labels.begin(), labels.size()};
        return CounterFamily<ValueType>(prefix, name, lbl_span, helptext, unit, is_sum);
    }

    /**
     * Accesses a counter instance. Creates the hosting metric family as well
     * as the counter lazily if necessary.
     * @param prefix The prefix (namespace) this family belongs to.
     * @param name The human-readable name of the metric, e.g., `requests`.
     * @param labels Values for all label dimensions of the metric.
     * @param helptext Short explanation of the metric.
     * @param unit Unit of measurement.
     * @param is_sum Indicates whether this metric accumulates something, where only the total value is of interest.
     * @param callback Passing a callback method will enable asynchronous mode. The callback method will be called by
     * the metrics subsystem whenever data is requested.
     */
    template<class ValueType = int64_t>
    std::shared_ptr<Counter<ValueType>> CounterInstance(std::string_view prefix, std::string_view name,
                                                        Span<const LabelView> labels, std::string_view helptext,
                                                        std::string_view unit = "", bool is_sum = false,
                                                        prometheus::CollectCallbackPtr callback = nullptr) {
        return WithLabelNames(labels, [&, this](auto labelNames) {
            auto family = CounterFamily<ValueType>(prefix, name, labelNames, helptext, unit, is_sum);
            return family->GetOrAdd(labels, callback);
        });
    }

    /// @copydoc counterInstance
    template<class ValueType = int64_t>
    std::shared_ptr<Counter<ValueType>> CounterInstance(std::string_view prefix, std::string_view name,
                                                        std::initializer_list<LabelView> labels,
                                                        std::string_view helptext, std::string_view unit = "",
                                                        bool is_sum = false,
                                                        prometheus::CollectCallbackPtr callback = nullptr) {
        auto lbl_span = Span{labels.begin(), labels.size()};
        return CounterInstance<ValueType>(prefix, name, lbl_span, helptext, unit, is_sum, callback);
    }

    /**
     * @return A gauge metric family. Creates the family lazily if necessary.
     * @param prefix The prefix (namespace) this family belongs to.
     * @param name The human-readable name of the metric, e.g., `requests`.
     * @param labels Names for all label dimensions of the metric.
     * @param helptext Short explanation of the metric.
     * @param unit Unit of measurement.
     * @param is_sum Indicates whether this metric accumulates something, where only the total value is of interest.
     * @param callback Passing a callback method will enable asynchronous mode. The callback method will be called by
     * the metrics subsystem whenever data is requested.
     */
    template<class ValueType = int64_t>
    auto GaugeFamily(std::string_view prefix, std::string_view name, Span<const std::string_view> labels,
                     std::string_view helptext, std::string_view unit = "", bool is_sum = false) {
        auto fam = LookupFamily(prefix, name);

        if constexpr ( std::is_same<ValueType, int64_t>::value ) {
            if ( fam )
                return std::static_pointer_cast<IntGaugeFamily>(fam);

            auto int_fam =
                std::make_shared<IntGaugeFamily>(prefix, name, labels, helptext, prometheus_registry, unit, is_sum);
            families.insert_or_assign(int_fam->FullName(), int_fam);
            return int_fam;
        }
        else {
            static_assert(std::is_same<ValueType, double>::value, "metrics only support int64_t and double values");
            if ( fam )
                return std::static_pointer_cast<DblGaugeFamily>(fam);

            auto dbl_fam =
                std::make_shared<DblGaugeFamily>(prefix, name, labels, helptext, prometheus_registry, unit, is_sum);
            families.insert_or_assign(dbl_fam->FullName(), dbl_fam);
            return dbl_fam;
        }
    }

    /// @copydoc GaugeFamily
    template<class ValueType = int64_t>
    auto GaugeFamily(std::string_view prefix, std::string_view name, std::initializer_list<std::string_view> labels,
                     std::string_view helptext, std::string_view unit = "", bool is_sum = false) {
        auto lbl_span = Span{labels.begin(), labels.size()};
        return GaugeFamily<ValueType>(prefix, name, lbl_span, helptext, unit, is_sum);
    }

    /**
     * Accesses a gauge instance. Creates the hosting metric family as well
     * as the gauge lazily if necessary.
     * @param prefix The prefix (namespace) this family belongs to.
     * @param name The human-readable name of the metric, e.g., `requests`.
     * @param labels Values for all label dimensions of the metric.
     * @param helptext Short explanation of the metric.
     * @param unit Unit of measurement.
     * @param is_sum Indicates whether this metric accumulates something, where only the total value is of interest.
     * @param callback Passing a callback method will enable asynchronous mode. The callback method will be called by
     * the metrics subsystem whenever data is requested.
     */
    template<class ValueType = int64_t>
    std::shared_ptr<Gauge<ValueType>> GaugeInstance(std::string_view prefix, std::string_view name,
                                                    Span<const LabelView> labels, std::string_view helptext,
                                                    std::string_view unit = "", bool is_sum = false,
                                                    prometheus::CollectCallbackPtr callback = nullptr) {
        return WithLabelNames(labels, [&, this](auto labelNames) {
            auto family = GaugeFamily<ValueType>(prefix, name, labelNames, helptext, unit, is_sum);
            return family->GetOrAdd(labels, callback);
        });
    }

    /// @copydoc GaugeInstance
    template<class ValueType = int64_t>
    std::shared_ptr<Gauge<ValueType>> GaugeInstance(std::string_view prefix, std::string_view name,
                                                    std::initializer_list<LabelView> labels, std::string_view helptext,
                                                    std::string_view unit = "", bool is_sum = false,
                                                    prometheus::CollectCallbackPtr callback = nullptr) {
        auto lbl_span = Span{labels.begin(), labels.size()};
        return GaugeInstance<ValueType>(prefix, name, lbl_span, helptext, unit, is_sum, callback);
    }

    // Forces the compiler to use the type `Span<const T>` instead of trying to
    // match parameters to a `span`.
    template<class T>
    struct ConstSpanOracle {
        using Type = Span<const T>;
    };

    // Convenience alias to safe some typing.
    template<class T>
    using ConstSpan = typename ConstSpanOracle<T>::Type;

    /**
     * Returns a histogram metric family. Creates the family lazily if
     * necessary.
     * @param prefix The prefix (namespace) this family belongs to. Usually the
     *               application or protocol name, e.g., `http`. The prefix `caf`
     *               as well as prefixes starting with an underscore are
     *               reserved.
     * @param name The human-readable name of the metric, e.g., `requests`.
     * @param labels Names for all label dimensions of the metric.
     * @param default_upper_bounds Upper bounds for the metric buckets.
     * @param helptext Short explanation of the metric.
     * @param unit Unit of measurement. Please use base units such as `bytes` or
     *             `seconds` (prefer lowercase). The pseudo-unit `1` identifies
     *             dimensionless counts.
     * @param is_sum Setting this to `true` indicates that this metric adds
     *               something up to a total, where only the total value is of
     *               interest. For example, the total number of HTTP requests.
     * @note The first call wins when calling this function multiple times with
     *       different bucket settings. Users may also override
     *       @p default_upper_bounds via run-time configuration.
     */
    template<class ValueType = int64_t>
    auto HistogramFamily(std::string_view prefix, std::string_view name, Span<const std::string_view> labels,
                         ConstSpan<ValueType> default_upper_bounds, std::string_view helptext,
                         std::string_view unit = "") {
        auto fam = LookupFamily(prefix, name);

        if constexpr ( std::is_same<ValueType, int64_t>::value ) {
            if ( fam )
                return std::static_pointer_cast<IntHistogramFamily>(fam);

            auto int_fam = std::make_shared<IntHistogramFamily>(prefix, name, labels, default_upper_bounds, helptext,
                                                                prometheus_registry, unit);
            families.insert_or_assign(int_fam->FullName(), int_fam);
            return int_fam;
        }
        else {
            static_assert(std::is_same<ValueType, double>::value, "metrics only support int64_t and double values");
            if ( fam )
                return std::static_pointer_cast<DblHistogramFamily>(fam);

            auto dbl_fam = std::make_shared<DblHistogramFamily>(prefix, name, labels, default_upper_bounds, helptext,
                                                                prometheus_registry, unit);
            families.insert_or_assign(dbl_fam->FullName(), dbl_fam);
            return dbl_fam;
        }
    }

    /// @copydoc HistogramFamily
    template<class ValueType = int64_t>
    auto HistogramFamily(std::string_view prefix, std::string_view name, std::initializer_list<std::string_view> labels,
                         ConstSpan<ValueType> default_upper_bounds, std::string_view helptext,
                         std::string_view unit = "") {
        auto lbl_span = Span{labels.begin(), labels.size()};
        return HistogramFamily<ValueType>(prefix, name, lbl_span, default_upper_bounds, helptext, unit);
    }

    /**
     * Returns a histogram. Creates the family lazily if necessary.
     * @param prefix The prefix (namespace) this family belongs to. Usually the
     *               application or protocol name, e.g., `http`. The prefix `caf`
     *               as well as prefixes starting with an underscore are
     *               reserved.
     * @param name The human-readable name of the metric, e.g., `requests`.
     * @param labels Names for all label dimensions of the metric.
     * @param default_upper_bounds Upper bounds for the metric buckets.
     * @param helptext Short explanation of the metric.
     * @param unit Unit of measurement. Please use base units such as `bytes` or
     *             `seconds` (prefer lowercase). The pseudo-unit `1` identifies
     *             dimensionless counts.
     * @param is_sum Setting this to `true` indicates that this metric adds
     *               something up to a total, where only the total value is of
     *               interest. For example, the total number of HTTP requests.
     * @note The first call wins when calling this function multiple times with
     *       different bucket settings. Users may also override
     *       @p default_upper_bounds via run-time configuration.
     */
    template<class ValueType = int64_t>
    std::shared_ptr<Histogram<ValueType>> HistogramInstance(std::string_view prefix, std::string_view name,
                                                            Span<const LabelView> labels,
                                                            ConstSpan<ValueType> default_upper_bounds,
                                                            std::string_view helptext, std::string_view unit = "") {
        return WithLabelNames(labels, [&, this](auto labelNames) {
            auto family = HistogramFamily<ValueType>(prefix, name, labelNames, default_upper_bounds, helptext, unit);
            return family->GetOrAdd(labels);
        });
    }

    /// @copdoc HistogramInstance
    template<class ValueType = int64_t>
    std::shared_ptr<Histogram<ValueType>> HistogramInstance(std::string_view prefix, std::string_view name,
                                                            std::initializer_list<LabelView> labels,
                                                            std::initializer_list<ValueType> default_upper_bounds,
                                                            std::string_view helptext, std::string_view unit = "") {
        auto lbls = Span{labels.begin(), labels.size()};
        auto bounds = Span{default_upper_bounds.begin(), default_upper_bounds.size()};
        return HistogramInstance<ValueType>(prefix, name, lbls, bounds, helptext, unit);
    }

    std::shared_ptr<MetricFamily> GetFamilyByFullName(const std::string& full_name) const {
        if ( auto it = families.find(full_name); it != families.end() )
            return it->second;
        return nullptr;
    }

    /**
     * @return A JSON description of the cluster configuration for reporting
     * to Prometheus for service discovery requests.
     */
    std::string GetClusterJson() const;

    /**
     * @return The pointer to the prometheus-cpp registry used by the telemetry
     * manager. This is public so that third parties (such as broker) can add
     * elements to it directly.
     */
    std::shared_ptr<prometheus::Registry> GetRegistry() const { return prometheus_registry; }

protected:
    template<class F>
    static auto WithLabelNames(Span<const LabelView> xs, F continuation) {
        if ( xs.size() <= 10 ) {
            std::string_view buf[10];
            for ( size_t index = 0; index < xs.size(); ++index )
                buf[index] = xs[index].first;

            return continuation(Span{buf, xs.size()});
        }
        else {
            std::vector<std::string_view> buf;
            for ( auto x : xs )
                buf.emplace_back(x.first);

            return continuation(Span{buf});
        }
    }

private:
    std::shared_ptr<MetricFamily> LookupFamily(std::string_view prefix, std::string_view name) const;

    std::shared_ptr<OtelReader> otel_reader;
    std::map<std::string, std::shared_ptr<MetricFamily>> families;

    detail::process_stats current_process_stats;
    double process_stats_last_updated = 0.0;

    std::shared_ptr<IntGauge> rss_gauge;
    std::shared_ptr<IntGauge> vms_gauge;
    std::shared_ptr<DblGauge> cpu_gauge;
    std::shared_ptr<IntGauge> fds_gauge;

    std::string endpoint_name;
    std::vector<std::string> export_prefixes;

    std::shared_ptr<prometheus::Registry> prometheus_registry;
    std::unique_ptr<prometheus::Exposer> prometheus_exposer;
};

} // namespace zeek::telemetry

namespace zeek {
extern telemetry::Manager* telemetry_mgr;

} // namespace zeek
