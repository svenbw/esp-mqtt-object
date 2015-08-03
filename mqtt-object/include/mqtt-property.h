#ifndef USER_PROPERTY_INCLUDE
#define USER_PROPERTY_INCLUDE

#include "user_interface.h"

struct object_config;
struct property;

typedef void(*property_cb_get) (const struct property* property, char** value, int* value_length);
typedef void(*property_cb_set) (const struct property* property, const char* value, const int value_length);

struct property_config {
	const char*     name;
	unsigned char   flags;

	property_cb_get get;
	property_cb_set set;
};

struct property {
	const struct property_config* config;
	const struct object* object;
};

void         ICACHE_FLASH_ATTR property_create_topic(const struct property* prop, char** topic);
void         ICACHE_FLASH_ATTR property_free_topic  (char** topic);
int          ICACHE_FLASH_ATTR is_property          (const struct property* property, const char* property_name);
const char*  ICACHE_FLASH_ATTR object_name          (const struct object* object);
#endif

