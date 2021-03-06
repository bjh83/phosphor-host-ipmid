#include "watchdog.hpp"
#include "utils.hpp"

#include <systemd/sd-bus.h>

#include <mapper.h>
#include <sdbusplus/bus.hpp>

extern sd_bus *bus;

struct set_wd_data_t {
    uint8_t timer_use;
    uint8_t timer_action;
    uint8_t preset;
    uint8_t flags;
    uint8_t ls;
    uint8_t ms;
}  __attribute__ ((packed));

static constexpr auto objname = "/xyz/openbmc_project/watchdog/host0";
static constexpr auto iface = "xyz.openbmc_project.State.Watchdog";
static constexpr auto property_iface = "org.freedesktop.DBus.Properties";

ipmi_ret_t ipmi_app_set_watchdog(
        ipmi_netfn_t netfn,
        ipmi_cmd_t cmd,
        ipmi_request_t request,
        ipmi_response_t response,
        ipmi_data_len_t data_len,
        ipmi_context_t context)
{
    sd_bus_message *reply = NULL;
    sd_bus_error error = SD_BUS_ERROR_NULL;
    int r = 0;

    set_wd_data_t *reqptr = (set_wd_data_t*) request;

    uint16_t timer = 0;

    // Making this uint64_t to match with provider
    uint64_t timer_ms = 0;
    char *busname = NULL;
    *data_len = 0;

    // Get number of 100ms intervals
    timer = (((uint16_t)reqptr->ms) << 8) + reqptr->ls;
    // Get timer value in ms
    timer_ms = timer * 100;

    printf("WATCHDOG SET Timer:[0x%X] 100ms intervals\n",timer);

    // Get bus name
    r = mapper_get_service(bus, objname, &busname);
    if (r < 0) {
        fprintf(stderr, "Failed to get %s bus name: %s\n",
                objname, strerror(-r));
        goto finish;
    }

    // Disable watchdog if running
    r = sd_bus_call_method(bus, busname, objname, property_iface,
                           "Set", &error, &reply, "ssv",
                           iface, "Enabled", "b", false);
    if(r < 0) {
        fprintf(stderr, "Failed to disable Watchdog: %s\n",
                    strerror(-r));
        goto finish;
    }

    /*
     * If the action is 0, it means, do nothing.  Multiple actions on timer
     * expiration aren't supported by phosphor-watchdog yet, so when the
     * action set is "none", we should just leave the timer disabled.
     */
    if (0 == reqptr->timer_action)
    {
        goto finish;
    }

    if (reqptr->timer_use & 0x40)
    {
        sd_bus_error_free(&error);
        reply = sd_bus_message_unref(reply);

        // Set the Interval for the Watchdog
        r = sd_bus_call_method(bus, busname, objname, property_iface,
                               "Set", &error, &reply, "ssv",
                               iface, "Interval", "t", timer_ms);
        if(r < 0) {
            fprintf(stderr, "Failed to set new expiration time: %s\n",
                    strerror(-r));
            goto finish;
        }

        // Now Enable Watchdog
        r = sd_bus_call_method(bus, busname, objname, property_iface,
                               "Set", &error, &reply, "ssv",
                               iface, "Enabled", "b", true);
        if(r < 0) {
            fprintf(stderr, "Failed to Enable Watchdog: %s\n",
                    strerror(-r));
            goto finish;
        }
    }

finish:
    sd_bus_error_free(&error);
    reply = sd_bus_message_unref(reply);
    free(busname);

    return (r < 0) ? -1 : IPMI_CC_OK;
}

ipmi_ret_t ipmi_app_reset_watchdog(
        ipmi_netfn_t netfn,
        ipmi_cmd_t cmd,
        ipmi_request_t request,
        ipmi_response_t response,
        ipmi_data_len_t data_len,
        ipmi_context_t context)
{
    sd_bus_message *reply = NULL;
    sd_bus_error error = SD_BUS_ERROR_NULL;
    int r = 0;
    char *busname = NULL;

    // Current time interval that is set in watchdog.
    uint64_t interval = 0;

    // Status code.
    ipmi_ret_t rc = IPMI_CC_OK;
    *data_len = 0;

    printf("WATCHDOG RESET\n");
    // Get bus name
    r = mapper_get_service(bus, objname, &busname);
    if (r < 0) {
        fprintf(stderr, "Failed to get %s bus name: %s\n",
                objname, strerror(-r));
        goto finish;
    }

    // Get the current interval and set it back.
    r = sd_bus_call_method(bus, busname, objname, property_iface,
                           "Get", &error, &reply, "ss",
                           iface, "Interval");

    if(r < 0) {
        fprintf(stderr, "Failed to get current Interval msg: %s\n",
                strerror(-r));
        goto finish;
    }

    // Now extract the value
    r = sd_bus_message_read(reply, "v", "t", &interval);
    if (r < 0) {
        fprintf(stderr, "Failed to read current interval: %s\n",
                strerror(-r));
        goto finish;
    }

    sd_bus_error_free(&error);
    reply = sd_bus_message_unref(reply);

    // Set watchdog timer
    r = sd_bus_call_method(bus, busname, objname, property_iface,
                           "Set", &error, &reply, "ssv",
                           iface, "TimeRemaining", "t", interval);
    if(r < 0) {
        fprintf(stderr, "Failed to refresh the timer: %s\n",
                strerror(-r));
        goto finish;
    }

finish:
    sd_bus_error_free(&error);
    reply = sd_bus_message_unref(reply);
    free(busname);

    return rc;
}
