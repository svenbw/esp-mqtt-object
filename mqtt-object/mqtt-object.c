#include "user_interface.h"
#include "osapi.h"
#include "os_type.h"
#include "mem.h"
#include "mqtt-object.h"
#include "debug.h"
#include "user_config.h"

struct object {
	const struct object_config* config;
	void* user_data;
	MQTT_Client* mqtt_client;
	ETSTimer daemon;
};

static struct object* global_objects;

static void ICACHE_FLASH_ATTR object_property_set_by_name(const struct object* object, const char* property_name, const char* data, const int data_length);


static void ICACHE_FLASH_ATTR object_daemon_task(void* _object)
{
	struct object* object = (struct object*) _object;

	object->config->daemon(object);
}

void ICACHE_FLASH_ATTR object_init_pool(const struct object_config** object_configs)
{
	struct object* object;
	const struct object_config** config;
	uint32_t cnt = 0, daemon_timeout_ms = 0;

	// Count the objects
	for(config=object_configs; *config != NULL; ++config, ++cnt);

	global_objects = (struct object*) os_zalloc(sizeof(struct object) * (cnt + 1));

	for(object=global_objects,config=object_configs; *config != NULL; ++object,++config)
	{
		object->config = *config;

		if(object->config->init != NULL)
			object->config->init(object, &daemon_timeout_ms);

		if((object->config->daemon != NULL) && (daemon_timeout_ms != 0))
		{
			os_timer_setfn(&object->daemon, object_daemon_task, object);
			os_timer_arm(&object->daemon, daemon_timeout_ms, 1);

			object_daemon_task(object);
		}
	}
}

void ICACHE_FLASH_ATTR object_deinit_pool(uint32_t* _mqtt_client)
{
	os_free(global_objects);
	global_objects = NULL;
}

void ICACHE_FLASH_ATTR object_connect_pool(uint32_t* _mqtt_client)
{
	struct object* object;
	const struct property_config* obj_prop;
	struct property* property = (struct property*)os_zalloc(sizeof(struct property));
	char *topic;

	for(object=global_objects; object->config != NULL; ++object)
	{
		object->mqtt_client = (MQTT_Client*)_mqtt_client;

		property->object = object;
	
		// Subscribe to all properties (with MQTT_SUB)
		for(obj_prop=object->config->properties; obj_prop->name != NULL; ++obj_prop)
		{
			if((obj_prop->flags & PROP_MQTT_SUB) != 0)
			{
				property->config = obj_prop;
				property_create_topic(property, &topic);

				MQTT_Subscribe((MQTT_Client*)_mqtt_client, topic, 0);

				property_free_topic(&topic);
			}
		}

		// Publish to all properties (with MQTT_PUB)
		for(obj_prop=object->config->properties; obj_prop->name != NULL; ++obj_prop)
		{
			if((obj_prop->flags & PROP_MQTT_PUB) != 0)
			{
				property->config = obj_prop;

				object_property_notify(property); 
			}
		}

		// Call the connect callback
		if(object->config->connect != NULL)
			object->config->connect(object);
	}

	os_free(property);
}

void ICACHE_FLASH_ATTR object_disconnect_pool(uint32_t* _mqtt_client)
{
	const struct object* object;

	for(object=global_objects; object->config != NULL; ++object)
	{
		if(object->config->disconnect != NULL)
			object->config->disconnect(object);
	}
}

void ICACHE_FLASH_ATTR object_mqtt_data(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len)
{
	const char *p, *begin;
	char *object_buf = NULL, *property_buf = NULL;
	int i;

	for(i=0,begin=topic,p=topic; i<=topic_len; ++i,++p)
	{
		if((*p=='/') || (*p=='\0') || (i==topic_len))
		{
			if((i>0) && (*p=='/') && *(p-1)=='/')
			{
				begin = p+1;
				continue;
			}

			if(begin != p)
			{
				char** topic_buf = NULL;

				if(object_buf == NULL)
					topic_buf = &object_buf;
				else if(property_buf == NULL)
					topic_buf = &property_buf;

				if(topic_buf)
				{
					*topic_buf = (char*)os_zalloc(p-begin+1);
					os_memcpy(*topic_buf, begin, p-begin);
				}

				begin = p+1;
			}
		}
	}

	const struct object* object = object_find_by_name(object_buf);

	if(object != NULL)
	{
		object_property_set_by_name(object, property_buf, data, data_len);
	}
	else
	{
		INFO("MQTTOBJECT: Object '%s' not found\r\n", object_buf);
	}

	if(object_buf)
		os_free(object_buf);
	if(property_buf)
		os_free(property_buf);
}

const struct ICACHE_FLASH_ATTR object* object_find_by_name(const char* name)
{
	const struct object* object;

	if(name != NULL)
	{
		for(object=global_objects; object != NULL; ++object)
		{
			if(strcmp(object_name(object), name) == 0)
				return object;
		}
	}

	return NULL;
}

struct property* ICACHE_FLASH_ATTR object_find_property_by_name(const struct object* object, const char* name)
{
	const struct property_config* obj_prop;

	if((name != NULL) && (object != NULL))
	{
		for(obj_prop=object->config->properties; obj_prop->name != NULL; ++obj_prop)
		{
			if(strcmp(obj_prop->name, name) == 0)
			{
				struct property* property = (struct property*)os_zalloc(sizeof(struct property));
				property->config = obj_prop;
				property->object = object;

				return property;
			}
		}
	}

	return NULL;
}

int ICACHE_FLASH_ATTR object_init_property_by_name(struct property* property, const struct object* object, const char* name)
{
	const struct property_config* obj_prop;

	if((property != NULL) && (name != NULL) && (object != NULL))
	{
		for(obj_prop=object->config->properties; obj_prop->name != NULL; ++obj_prop)
		{
			if(strcmp(obj_prop->name, name) == 0)
			{
				property->config = obj_prop;
				property->object = object;

				return 0;
			}
		}
	}

	return 1;
}

static void ICACHE_FLASH_ATTR object_property_set_by_name(const struct object* object, const char* property_name, const char* data, const int data_length)
{
	char* data_buf;
	struct property* property = object_find_property_by_name(object, property_name);

	if(property == NULL)
	{
		INFO("MQTTOBJECT: Property '%s' not found\r\n", property_name);
		return;
	}

	data_buf = (char*)os_zalloc(data_length+1);
	os_memcpy(data_buf, data, data_length);

	if(property->config->set != NULL)
		property->config->set(property, data_buf, data_length);

	else if(object->config->set != NULL)
		object->config->set(property, data_buf, data_length);

	os_free(data_buf);

	object_free_property(&property);
}

void ICACHE_FLASH_ATTR object_property_notify(const struct property* property)
{
	int data_length;
	char *topic, *data = NULL;

	if(property->object->mqtt_client == NULL)
		return;

	if(property->config->get != NULL)
	{
		property->config->get(property, NULL, &data_length);
		if(data_length > 0)
		{
			data = (char*)os_zalloc(data_length);
			property->config->get(property, &data, &data_length);
		}
	}
	else if(property->object->config->get != NULL)
	{
		property->object->config->get(property, NULL, &data_length);
		if(data_length > 0)
		{
				data = (char*)os_zalloc(data_length);
				property->object->config->get(property, &data, &data_length);
		}
	}

	if(data != NULL)
	{
		property_create_topic(property, &topic);
		MQTT_Publish(property->object->mqtt_client, topic, data, data_length, 0, 0);
		property_free_topic(&topic);
		os_free(data);
	}
}

void ICACHE_FLASH_ATTR object_free_property(struct property** property)
{
	if(*property != NULL)
	{
		os_free(*property);
		*property = NULL;
	}
}

const char* ICACHE_FLASH_ATTR object_name(const struct object* object)
{
	return object->config->name;
}

void ICACHE_FLASH_ATTR object_set_user_data(struct object* object, void* data)
{
	object->user_data = data;
}

void* ICACHE_FLASH_ATTR object_get_user_data(const struct object* object)
{
	return object->user_data;
}

MQTT_Client* ICACHE_FLASH_ATTR object_get_mqtt_client(const struct object* object)
{
	return object->mqtt_client;
}
