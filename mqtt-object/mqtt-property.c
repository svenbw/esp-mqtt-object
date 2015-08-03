#include "user_interface.h"
#include "osapi.h"
#include "os_type.h"
#include "mem.h"
#include "mqtt-object.h"
#include "mqtt-property.h"
#include "debug.h"
#include "user_config.h"

int ICACHE_FLASH_ATTR is_property(const struct property* property, const char* property_name)
{
	if(property == NULL)
		return 0;

	return (strcmp(property->config->name, property_name) == 0);
}

void ICACHE_FLASH_ATTR property_create_topic(const struct property* prop, char** topic)
{
	size_t len = os_strlen(object_name(prop->object)) + 1 + os_strlen(prop->config->name);

	*topic = (char*)os_zalloc(len+1);

	strcat(*topic, object_name(prop->object));
	strcat(*topic, "/");
	strcat(*topic, prop->config->name);
}

void ICACHE_FLASH_ATTR property_free_topic(char** topic)
{
	os_free(*topic);

	*topic = NULL;
}

const char* ICACHE_FLASH_ATTR property_name(const struct property* property)
{
	return property->config->name;
}

