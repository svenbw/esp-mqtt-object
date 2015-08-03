**esp-object**
==============

This is a support library for ESP8266 modules and allows the creation of modular MQTT objects under the [MQTT client library for ESP8266](https://github.com/tuanpmt/esp_mqtt)

**Features**
- Create objects with a list of properties
- Connect and disconnect callbacks for each object
- Automatic topic generation (object_name/property_name)
- Object level and property level setters and getters
- Daemon function (periodically function invoked function for each object)
- Allow modular clean code design

**Installation**

One option is to add the source as a submodule to the existing git directory 
```
#Go to the esp_mqtt directory
cd esp_mqtt

#Create a submodule in the existing project
git submodule add https://github.com/svenbw/esp-mqtt-object esp8266-mqtt-object

#Add a symbolic link to the source files of the library
ln -s esp-mqtt-object/mqtt-object mqtt-object
```

Another option is to clone the project and copy the esp-mqtt-object-directory and files to the esp8266_mqtt directory
```
#Go to the esp_mqtt directory
cd esp_mqtt

#Copy the files
cp -R ../esp-mqtt-object/mqtt-object .
```

**Configuration**

Modify the original Makefile to include the esp-mqtt-object project.
Open the Makefile with your favorite editor and add look for the MODULES definition and add the name of the symbolic link or directory name (mqtt-object).
```
#which modules (subdirectories) of the project to include in compiling
MODULES    = driver mqtt user modules mqtt-object
```

**Object creation**
```
#include "osapi.h"
#include "mem.h"
#include "mqtt-object.h"
#include "mqtt-property.h"
#include "config.h"
#include "debug.h"

struct simple_io_data {
	int counter;
};

/**
  * @brief  Called when the object pool is initialised
  * @param  object: contain a pointer to the object
  * @param  daemon_timeout_ms: timeout to run the daemon function
  * @retval None
  */
void simple_io_init(struct object* object, uint32_t* daemon_timeout_ms)
{
	INFO("SIMPLE IO: Init %s\r\n", object_name(object));

	// Create a custom structure with private data
	struct simple_io_data* data = (struct simple_io_data*) os_zalloc(sizeof(struct simple_io_data));

	// Assign it to the object for later use
	object_set_user_data(object, data);

	// Do something with it
	data->counter = 0;

	// Run the daemon every 1000ms
	// Not setting this value or assigning it to 0 will not start the daemon
	*daemon_timeout_ms = 1000;
}

/**
  * @brief  Called when the MQTT library invokes the MQTT_OnConnected callback
  * @param  object: contain a pointer to the object
  * @retval None
  */
void simple_io_connect(const struct object* object)
{
	INFO("SIMPLE IO: Connected\r\n");
}

/**
  * @brief  Called when the MQTT library invokes the MQTT_OnDisconnected callback
  * @param  object: contain a pointer to the object
  * @retval None
  */
void simple_io_disconnect(const struct object* object)
{
	INFO("SIMPLE IO: Connected\r\n");
}

/**
  * @brief  The daemon is the main loop of the object, it is invoked by an os_timer
  * @param  object: contain a pointer to the object
  * @retval None
  */
void simple_io_daemon(const struct object* object)
{
	struct simple_io_data* data = object_get_user_data(object);

	INFO("SIMPLE IO: Daemon invoked\r\n");

	data->counter++;
}

/**
  * @brief  Object getter: Assigns a value of a property(!) before it gets published
  * @param  property: contain a pointer to the property that will be published
  * @param  value: pointer to the value of the property
  * @param  value_length: length of the returned value (can be longer)
  * @retval None
  */
void simple_io_get(const struct property* property, char** value, int* value_length)
{
	// Getters are invoked with value == NULL to find out how many memory needs to be allocated
	if(value == NULL)
	{
		// 4 bytes allows on and off assignments
		*value_length = 4;
		return;
	}

	// Since this handler is invoked on every property without it's own handler the name
	// of the property should be checked with a string to return the correct value
	if(is_property(property, "in2"))
		os_strcpy(*value, "off");
}

/**
  * @brief  Object setter: Invoked on a property publish
  * @param  property: contain a pointer to the property
  * @param  value: pointer to the published value
  * @param  value_length: length of the received value
  * @retval None
  */
void simple_io_set(const struct property* property, const char* value, const int value_length)
{
	// Object level setters are invoked when there is no setter on the property level
	struct simple_io_data* data = (struct simple_io_data*) object_get_user_data(property->object);

	INFO("SIMPLE IO: Object set: %s.%s = %s (@%d)\r\n", object_name(property->object), property->config->name, value, data->counter);
}

/**
  * @brief  Property setter: Invoked on a property publish
  * @param  property: contain a pointer to the property
  * @param  value: pointer to the published value
  * @param  value_length: length of the received value
  * @retval None
  */
void simple_io_out1_set(const struct property* property, const char* value, const int value_length)
{
	// Property level setter
	struct simple_io_data* data = (struct simple_io_data*) object_get_user_data(property->object);

	INFO("SIMPLE IO: Property set: %s.%s = %s (@%d)\r\n", object_name(property->object), property->config->name, value, data->counter);
}

/**
  * @brief  Property getter: Assigns a value before it gets published
  * @param  property: contain a pointer to the property that will be published
  * @param  value: pointer to the value of the property
  * @param  value_length: length of the returned value (can be longer)
  * @retval None
  */
void simple_io_in1_get(const struct property* property, char** value, int* value_length)
{
	// Getters are invoked with value == NULL to find out how many memory needs to be allocated
	if(value == NULL)
	{
		// 4 bytes allows on and off assignments
		*value_length = 4;
		return;
	}

	// The second call, the memory to copy the data in is available and can be used
	// (Cleaning up is done by the higher layer)
	os_strcpy(*value, "on");
}


// Define the simple io object
// Assign the name and each of the properties
// Also bind the callbacks to the configuration
const struct object_config my_simple_io = {
	.name = "simpleio",
	.init = simple_io_init,
	.connect = simple_io_connect,
	.disconnect = simple_io_disconnect,
	.get = simple_io_get,
	.set = simple_io_set,
	.properties = {
		{
			.name = "out1",
			.flags = PROP_MQTT_SUB
		},
		{
			.name = "out2",
			.set = simple_io_out1_set,
			.flags = PROP_MQTT_SUB
		},
		{
			.name = "in1",
			.get = simple_io_in1_get,
			.flags = PROP_MQTT_PUB
		},
		{
			.name = "in2",
			.flags = PROP_MQTT_PUB
		},
		{
			.name = NULL
		}
	}
};
```

**Modified user_main.c**

```
// NEW: Create a list of objects (only one in this example)
extern const struct object_config my_simple_io;
const struct object_config* objects[] = {
	&my_simple_io,
	NULL
};

...

void user_init(void)
{
	uart_init(BIT_RATE_115200, BIT_RATE_115200);
	os_delay_us(1000000);

	CFG_Load();

	// NEW: Initialise the object pool (will invoke object_init functions)
	object_init_pool(objects);

	MQTT_InitConnection(&mqttClient, sysCfg.mqtt_host, sysCfg.mqtt_port, sysCfg.security);
	//MQTT_InitConnection(&mqttClient, "192.168.11.122", 1880, 0);

	MQTT_InitClient(&mqttClient, sysCfg.device_id, sysCfg.mqtt_user, sysCfg.mqtt_pass, sysCfg.mqtt_keepalive, 1);
	//MQTT_InitClient(&mqttClient, "client_id", "user", "pass", 120, 1);

	MQTT_InitLWT(&mqttClient, "/lwt", "offline", 0, 0);

	// NEW: replace all callbacks with the callbacks provided by the library
	MQTT_OnConnected(&mqttClient, object_connect_pool);
	MQTT_OnDisconnected(&mqttClient, object_disconnect_pool);
	MQTT_OnPublished(&mqttClient, mqttPublishedCb);
	MQTT_OnData(&mqttClient, object_mqtt_data);

	WIFI_Connect(sysCfg.sta_ssid, sysCfg.sta_pwd, wifiConnectCb);

	INFO("\r\nSystem started ...\r\n");
}
```
