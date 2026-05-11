/*
 * Virtual battery sensor for split keyboards.
 * Reports MIN(local_soc, peripheral_soc) so macOS sees the lower of the two
 * halves rather than only the central's level.
 *
 * Only instantiated on the central (left) half via caldera_left.overlay.
 * The peripheral_battery_soc global is updated by the ZMK event listener
 * and initialised to 100 so we never report a false 0% before the right
 * half has connected and sent its first reading.
 */

#define DT_DRV_COMPAT zmk_min_split_battery

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#include <zmk/split/central.h>
#endif

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* Tracks the most-recently received peripheral SOC; 100 until first report. */
static uint8_t peripheral_battery_soc = 100;

static int peripheral_battery_listener(const zmk_event_t *eh) {
    const struct zmk_peripheral_battery_state_changed *ev =
        as_zmk_peripheral_battery_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }
    if (ev->source == 0) {
        peripheral_battery_soc = ev->state_of_charge;
        LOG_DBG("Peripheral battery updated: %d%%", peripheral_battery_soc);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(min_split_peripheral_battery, peripheral_battery_listener);
ZMK_SUBSCRIPTION(min_split_peripheral_battery, zmk_peripheral_battery_state_changed);

struct min_split_battery_config {
    const struct device *source;
};

static int min_split_battery_sample_fetch(const struct device *dev, enum sensor_channel chan) {
    const struct min_split_battery_config *cfg = dev->config;

    if (chan != SENSOR_CHAN_GAUGE_STATE_OF_CHARGE && chan != SENSOR_CHAN_ALL) {
        return -ENOTSUP;
    }

    return sensor_sample_fetch_chan(cfg->source, SENSOR_CHAN_GAUGE_STATE_OF_CHARGE);
}

static int min_split_battery_channel_get(const struct device *dev, enum sensor_channel chan,
                                         struct sensor_value *val) {
    const struct min_split_battery_config *cfg = dev->config;

    if (chan != SENSOR_CHAN_GAUGE_STATE_OF_CHARGE) {
        return -ENOTSUP;
    }

    struct sensor_value local_val;
    int rc = sensor_channel_get(cfg->source, SENSOR_CHAN_GAUGE_STATE_OF_CHARGE, &local_val);
    if (rc != 0) {
        return rc;
    }

    uint8_t local_soc = (uint8_t)local_val.val1;
    uint8_t peripheral_soc = peripheral_battery_soc;

    uint8_t reported = MIN(local_soc, peripheral_soc);
    LOG_DBG("Battery: left=%d%% right=%d%% reporting=%d%%", local_soc, peripheral_soc, reported);

    val->val1 = reported;
    val->val2 = 0;
    return 0;
}

static const struct sensor_driver_api min_split_battery_api = {
    .sample_fetch = min_split_battery_sample_fetch,
    .channel_get = min_split_battery_channel_get,
};

static int min_split_battery_init(const struct device *dev) {
    const struct min_split_battery_config *cfg = dev->config;

    if (!device_is_ready(cfg->source)) {
        LOG_ERR("Source battery device not ready");
        return -ENODEV;
    }

    return 0;
}

#define MIN_SPLIT_BATTERY_INST(n)                                                                  \
    static const struct min_split_battery_config min_split_battery_config_##n = {                 \
        .source = DEVICE_DT_GET(DT_INST_PHANDLE(n, source)),                                      \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(n, &min_split_battery_init, NULL, NULL,                                 \
                          &min_split_battery_config_##n, POST_KERNEL,                              \
                          91, &min_split_battery_api);

DT_INST_FOREACH_STATUS_OKAY(MIN_SPLIT_BATTERY_INST)
