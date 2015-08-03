#ifndef USER_OBJECT_INCLUDE
#define USER_OBJECT_INCLUDE

#include "user_interface.h"
#include "os_type.h"
#include "mqtt-property.h"
#include "mqtt.h"

#define PROP_MQTT_PUB 1
#define PROP_MQTT_SUB 2


struct object;

typedef void(*object_cb_init)       (struct object* object, uint32_t* daemon_timeout_ms);
typedef void(*object_cb_connect)    (const struct object* object);
typedef void(*object_cb_disconnect) (const struct object* object);
typedef void(*object_cb_daemon)     (const struct object* object);
typedef void(*object_cb_get)        (const struct property* property, char** value, int* value_length);
typedef void(*object_cb_set)        (const struct property* property, const char* value, const int value_length);

struct object_config {
	const char*            name;
	object_cb_init         init;
	object_cb_connect      connect;
	object_cb_disconnect   disconnect;
	object_cb_daemon       daemon;
	object_cb_get          get;
	object_cb_set          set;

	struct property_config properties[];
};

void ICACHE_FLASH_ATTR object_init_pool(const struct object_config** object_configs);
void ICACHE_FLASH_ATTR object_deinit_pool();
void ICACHE_FLASH_ATTR object_connect_pool(uint32_t* _mqtt_client);
void ICACHE_FLASH_ATTR object_disconnect_pool(uint32_t* _mqtt_client);
void ICACHE_FLASH_ATTR object_mqtt_data(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len);

void ICACHE_FLASH_ATTR object_property_notify(const struct property* property);

const struct object* ICACHE_FLASH_ATTR object_find_by_name(const char* name);
const char* ICACHE_FLASH_ATTR object_name(const struct object* object);
void ICACHE_FLASH_ATTR object_set_user_data(struct object* object, void* data);
void* ICACHE_FLASH_ATTR object_get_user_data(const struct object* object);
MQTT_Client* ICACHE_FLASH_ATTR object_get_mqtt_client(const struct object* object);
struct property* ICACHE_FLASH_ATTR object_find_property_by_name(const struct object* object, const char* name);
int ICACHE_FLASH_ATTR object_init_property_by_name(struct property* property, const struct object* object, const char* name);
void ICACHE_FLASH_ATTR object_free_property(struct property** property);

#endif
