#include <application.h>
#include <bc_onewire_relay.h>

#define TEMPERATURE_MEASURE_INTERVAL_SECOND (30)
#define TEMPERATURE_PUBLISH_DELTA (1.0f)
#define TEMPERATURE_PUBLISH_TIMEOUT_SECOND (15 * 60)

// LED instance
bc_led_t led;

// Button instance
bc_button_t button;

// Thermometer instance
bc_tmp112_t tmp112;
float publish_temperature = NAN;
bc_tick_t temperature_publish_timeout = 0;

// 1-wire relay instance
bc_onewire_relay_t relay;

void radio_pub_state(bc_onewire_relay_channel_t channel);
void denkovi_ralay_set(uint64_t *id, const char *topic, void *value, void *param);
void denkovi_ralay_get(uint64_t *id, const char *topic, void *value, void *param);

// lut for publish state to mqtt over radio
static const char topics[8][23] =
{
    [BC_ONEWIRE_RELAY_CHANNEL_Q1] = "denkovi-relay/q1/state",
    [BC_ONEWIRE_RELAY_CHANNEL_Q2] = "denkovi-relay/q2/state",
    [BC_ONEWIRE_RELAY_CHANNEL_Q3] = "denkovi-relay/q3/state",
    [BC_ONEWIRE_RELAY_CHANNEL_Q4] = "denkovi-relay/q4/state",
    [BC_ONEWIRE_RELAY_CHANNEL_Q5] = "denkovi-relay/q5/state",
    [BC_ONEWIRE_RELAY_CHANNEL_Q6] = "denkovi-relay/q6/state",
    [BC_ONEWIRE_RELAY_CHANNEL_Q7] = "denkovi-relay/q7/state",
    [BC_ONEWIRE_RELAY_CHANNEL_Q8] = "denkovi-relay/q8/state",
};

// subscribe table, format: topic, expect payload type, callback, user param
static const bc_radio_sub_t subs[] = {
    // state/set
    {"denkovi-relay/q1/state/set", BC_RADIO_SUB_PT_BOOL, denkovi_ralay_set, (void *) BC_ONEWIRE_RELAY_CHANNEL_Q1},
    {"denkovi-relay/q2/state/set", BC_RADIO_SUB_PT_BOOL, denkovi_ralay_set, (void *) BC_ONEWIRE_RELAY_CHANNEL_Q2},
    {"denkovi-relay/q3/state/set", BC_RADIO_SUB_PT_BOOL, denkovi_ralay_set, (void *) BC_ONEWIRE_RELAY_CHANNEL_Q3},
    {"denkovi-relay/q4/state/set", BC_RADIO_SUB_PT_BOOL, denkovi_ralay_set, (void *) BC_ONEWIRE_RELAY_CHANNEL_Q4},
    {"denkovi-relay/q5/state/set", BC_RADIO_SUB_PT_BOOL, denkovi_ralay_set, (void *) BC_ONEWIRE_RELAY_CHANNEL_Q5},
    {"denkovi-relay/q6/state/set", BC_RADIO_SUB_PT_BOOL, denkovi_ralay_set, (void *) BC_ONEWIRE_RELAY_CHANNEL_Q6},
    {"denkovi-relay/q7/state/set", BC_RADIO_SUB_PT_BOOL, denkovi_ralay_set, (void *) BC_ONEWIRE_RELAY_CHANNEL_Q7},
    {"denkovi-relay/q8/state/set", BC_RADIO_SUB_PT_BOOL, denkovi_ralay_set, (void *) BC_ONEWIRE_RELAY_CHANNEL_Q8},
    // state/get
    {"denkovi-relay/q1/state/get", BC_RADIO_SUB_PT_NULL, denkovi_ralay_get, (void *) BC_ONEWIRE_RELAY_CHANNEL_Q1},
    {"denkovi-relay/q2/state/get", BC_RADIO_SUB_PT_NULL, denkovi_ralay_get, (void *) BC_ONEWIRE_RELAY_CHANNEL_Q2},
    {"denkovi-relay/q3/state/get", BC_RADIO_SUB_PT_NULL, denkovi_ralay_get, (void *) BC_ONEWIRE_RELAY_CHANNEL_Q3},
    {"denkovi-relay/q4/state/get", BC_RADIO_SUB_PT_NULL, denkovi_ralay_get, (void *) BC_ONEWIRE_RELAY_CHANNEL_Q4},
    {"denkovi-relay/q5/state/get", BC_RADIO_SUB_PT_NULL, denkovi_ralay_get, (void *) BC_ONEWIRE_RELAY_CHANNEL_Q5},
    {"denkovi-relay/q6/state/get", BC_RADIO_SUB_PT_NULL, denkovi_ralay_get, (void *) BC_ONEWIRE_RELAY_CHANNEL_Q6},
    {"denkovi-relay/q7/state/get", BC_RADIO_SUB_PT_NULL, denkovi_ralay_get, (void *) BC_ONEWIRE_RELAY_CHANNEL_Q7},
    {"denkovi-relay/q8/state/get", BC_RADIO_SUB_PT_NULL, denkovi_ralay_get, (void *) BC_ONEWIRE_RELAY_CHANNEL_Q8}
};

void radio_pub_state(bc_onewire_relay_channel_t channel)
{
    bool state;

    if (bc_onewire_relay_get_state(&relay, channel, &state))
    {
        bc_radio_pub_bool(topics[channel], &state);
    }
    else
    {
        bc_radio_pub_bool(topics[channel], NULL);
    }
}

void denkovi_ralay_set(uint64_t *id, const char *topic, void *value, void *param)
{
    bc_led_pulse(&led, 30);

    uint32_t channel = (uint32_t) param;

    bc_onewire_relay_set_state(&relay, channel, *(bool *) value);

    radio_pub_state(channel);
}

void denkovi_ralay_get(uint64_t *id, const char *topic, void *value, void *param)
{
    bc_led_pulse(&led, 30);

    uint32_t channel = (uint32_t) param;

    radio_pub_state(channel);
}

void button_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param)
{
    static uint16_t event_count = 0;

    if (event == BC_BUTTON_EVENT_PRESS)
    {
        bc_led_pulse(&led, 100);

        event_count++;

        bc_radio_pub_push_button(&event_count);
    }
}

void tmp112_event_handler(bc_tmp112_t *self, bc_tmp112_event_t event, void *event_param)
{
    float temperature;

    if (event == BC_TMP112_EVENT_UPDATE)
    {
        if (bc_tmp112_get_temperature_celsius(self, &temperature))
        {
            if ((fabsf(temperature - publish_temperature) >= TEMPERATURE_PUBLISH_DELTA) || (temperature_publish_timeout < bc_scheduler_get_spin_tick()))
            {
                // Used same MQTT topic as for Temperature Tag with alternate i2c address
                bc_radio_pub_temperature(BC_RADIO_PUB_CHANNEL_R1_I2C0_ADDRESS_ALTERNATE, &temperature);

                publish_temperature = temperature;

                temperature_publish_timeout = bc_tick_get() + (TEMPERATURE_PUBLISH_TIMEOUT_SECOND * 1000);
            }
        }
    }
}

void application_init(void)
{
    // Initialize LED
    bc_led_init(&led, BC_GPIO_LED, false, false);
    bc_led_set_mode(&led, BC_LED_MODE_OFF);

    // Initialize radio
    bc_radio_init(BC_RADIO_MODE_NODE_LISTENING);
    bc_radio_set_subs((bc_radio_sub_t *) subs, sizeof(subs)/sizeof(bc_radio_sub_t));

    // Initialize button
    bc_button_init(&button, BC_GPIO_BUTTON, BC_GPIO_PULL_DOWN, false);
    bc_button_set_event_handler(&button, button_event_handler, NULL);

    // Initialize thermometer sensor on core module
    bc_tmp112_init(&tmp112, BC_I2C_I2C0, 0x49);
    bc_tmp112_set_event_handler(&tmp112, tmp112_event_handler, NULL);
    bc_tmp112_set_update_interval(&tmp112, TEMPERATURE_PUBLISH_TIMEOUT_SECOND);

    // Initialize sensor module
    bc_module_sensor_init();
    // Pull Up for 1 wire
    bc_module_sensor_set_pull(BC_MODULE_SENSOR_CHANNEL_A, BC_MODULE_SENSOR_PULL_UP_4K7);

    // Initialize onewire relay
    bc_onewire_relay_init(&relay, BC_GPIO_P4, 0x00);

    bc_radio_pairing_request("bcf-denkovi-1wire-relay", VERSION);

    bc_led_pulse(&led, 2000);

    bc_scheduler_plan_current_relative(2000);
}

uint8_t radio_pub_state_idx = 0;

void application_task(void)
{
    radio_pub_state(radio_pub_state_idx++);

    if (radio_pub_state_idx < 8)
    {
        bc_scheduler_plan_current_relative(200);
    }
}
