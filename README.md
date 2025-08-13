# MCP over MQTT SDK

[English](README.md) | [中文](README_CN.md)

An implementation of Model Context Protocol (MCP) over MQTT 5.0 with paho-mqtt-c library.

## What is MCP?

**Model Context Protocol (MCP)** is an open protocol for secure, structured communication between AI assistants and external data sources and tools. MCP allows AI assistants to:

- Access external data sources and knowledge bases
- Invoke external tools and services
- Operate within secure boundaries
- Interact with multiple data sources in a standardized way

MCP defines clear interfaces and protocols, enabling AI assistants to safely extend their capabilities while maintaining control over data access.

## Why MCP over MQTT?
MQTT is a lightweight messaging protocol designed for IoT and edge computing. Combining MCP with MQTT brings the following advantages:

### Core Features
- **Lightweight Transport**: Suitable for bandwidth-constrained network environments
- **Reliable Message Delivery**: Supports QoS levels and message persistence
- **Built-in Service Discovery**: MCP clients can automatically discover available MCP servers
- **Load Balancing and Scalability**: Supports horizontal scaling of MCP server instances
- **Flexible Access Control**: Implements fine-grained authorization through MQTT topic permissions

### Use Cases
- **Edge Computing**: Deploy MCP servers on resource-constrained devices
- **IoT Applications**: Provide AI capabilities for smart devices
- **Cloud-Edge Collaboration**: Seamless integration between cloud AI and edge devices
- **Real-time Communication**: Support low-latency AI tool invocation

## SDK Features

### Core Functionality
- **MCP Server**: Complete MCP protocol implementation
- **MQTT 5.0 Support**: Based on ESP-IDF MQTT client
- **Tool Registration**: Support dynamic registration and invocation of MCP tools
- **Resource Management**: Provide data resource access capabilities
- **JSON-RPC**: Communication protocol based on JSON-RPC 2.0

### Tool System
```c
typedef struct {
    char *name;                    // Tool name
    char *description;             // Tool description
    int property_count;            // Number of parameters
    property_t *properties;        // Parameter definitions
    const char *(*call)(int n_args, property_t *args); // Execution function
} mcp_tool_t;
```

### Resource Management
```c
typedef struct {
    char *uri;                     // Resource URI
    char *name;                    // Resource name
    char *description;             // Resource description
    char *mime_type;               // MIME type
    char *title;                   // Resource title
} mcp_resource_t;
```

### Security Features
- **TLS/SSL Support**: Support for MQTTS secure connections
- **User Authentication**: Username/password authentication
- **Certificate Verification**: Client certificate support
- **Access Control**: Permission management based on MQTT topics

## Installation and Dependencies

### System Requirements
- MQTT Broker supporting MQTT 5.0

### Dependencies

[MQTT C Library](https://github.com/eclipse-paho/paho.mqtt.c)
```shell
$ git clone https://github.com/eclipse-paho/paho.mqtt.c
$ cd paho.mqtt.c && mkdir build && cd build
$ cmake .. && make && sudo make install
```

[cjson](https://github.com/DaveGamble/cJSON)
```shell
$ git clone https://github.com/DaveGamble/cJSON
$ cd cJSON && mkdir build && cd build
$ cmake ..&& make && sudo make install
```

### Build and Install Library

```shell
$ git clone https://github.com/mqtt-ai/paho-mcp-over-mqtt
$ cd paho-mcp-over-mqtt && mkdir build && cd build
$ cmake .. && make && sudo make install
```

## Quick Start

### Basic Usage Example

```c
#include "mcp_server.h"

// Define MCP tools
mcp_tool_t my_tools[] = {
    {
        .name = "get_temperature",
        .description = "Get device temperature",
        .property_count = 0,
        .properties = NULL,
        .call = get_temperature_callback
    }
};

// Initialize MCP server
mcp_server_t *server = mcp_server_init(
    "sensor",           // Server name
    "Sensor MCP Server", // Description
    "mqtt://broker.example.com", // MQTT Broker URI
    "client_001",       // Client ID
    "username",               // Username
    "password",               // Password
    NULL                      // Certificate (optional)
);

// Register tools
mcp_server_register_tool(server, 1, my_tools);

// Start server
mcp_server_run(server);
```

### Tool Callback Function Example

```c
const char* get_temperature_callback(int n_args, property_t *args) {
    // Read sensor data
    float temp = read_temperature_sensor();
    
    // Return JSON formatted result
    static char result[64];
    snprintf(result, sizeof(result), "{\"temperature\": %.2f}", temp);
    return result;
}
```

## Protocol Specification

This SDK implements the [MCP over MQTT protocol specification](https://github.com/mqtt-ai/mcp-over-mqtt), supporting:

- **Service Discovery**: Automatic discovery and registration of MCP servers
- **Load Balancing**: Support for multi-instance deployment
- **State Management**: Maintain MCP server state
- **Access Control**: Permission control based on MQTT topics

## License

This project is licensed under the [LICENSE](LICENSE) license.

## Related Links

- [MCP Official Documentation](https://modelcontextprotocol.io/)
- [MCP over MQTT Specification](https://github.com/mqtt-ai/mcp-over-mqtt)
- [MQTT Protocol Specification](https://mqtt.org/specification/)
- [MCP over MQTT Python SDK](https://github.com/emqx/mcp-python-sdk) 