#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <MQTTAsync.h>

#include "jsonrpc.h"
#include "mcp_server.h"

typedef struct {
    char *client_id;
} connect_mcp_client;

struct mcp_server {
    char *name;
    char *description;

    char *broker_uri;
    char *client_id;
    char *user;
    char *password;
    char *cert;

    int         n_tools;
    mcp_tool_t *tools;

    int               n_resources;
    mcp_resource_t   *resources;
    mcp_resource_read read_callback;

    MQTTAsync                client;
    MQTTAsync_connectOptions conn_opts;
    MQTTAsync_willOptions    will_opts;

    char *control_topic;
    char *presence_topic;
    char *capability_topic;

    int                 n_clients;
    connect_mcp_client *clients;
};

MQTTProperty property = {
    .identifier  = MQTTPROPERTY_CODE_USER_PROPERTY,
    .value.data  = { .len = 18, .data = "MCP-COMPONENT-TYPE" },
    .value.value = { .len = 10, .data = "mcp-server" },
};

MQTTProperties props = MQTTProperties_initializer;

int msg_arrvd(void *ctx, char *topic, int topicLen, MQTTAsync_message *message);

void conn_lost(void *ctx, char *cause)
{
    mcp_server_t *server = (mcp_server_t *) ctx;
    int           rc;

    MQTTAsync_connect(server->client, &server->conn_opts);
}

void onConnectFailure(void *ctx, MQTTAsync_failureData5 *response)
{
    (void) ctx;
    printf("Connection failed, rc %d\n", response->code);
}

void onConnect(void *ctx, MQTTAsync_successData5 *response)
{
    (void) response;
    mcp_server_t *server = (mcp_server_t *) ctx;

    int ret =
        MQTTAsync_subscribe(server->client, server->control_topic, 0, NULL);
    printf("Connected to MQTT broker: %s, %d\n", server->broker_uri, ret);

    char *data = jsonrpc_encode(
        jsonrpc_server_online(server->name, server->description, 0, NULL));

    MQTTAsync_message online_msg = MQTTAsync_message_initializer;
    online_msg.payload           = (void *) data;
    online_msg.payloadlen        = (int) strlen(online_msg.payload);
    online_msg.qos               = 0;
    online_msg.retained          = 1;

    MQTTAsync_sendMessage(server->client, server->presence_topic, &online_msg,
                          NULL);
    free(data);
}

mcp_server_t *mcp_server_init(const char *name, const char *description,
                              const char *broker_uri, const char *client_id,
                              const char *user, const char *password,
                              const char *cert)
{
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer5;
    MQTTAsync_willOptions    will_opts = MQTTAsync_willOptions_initializer;
    MQTTAsync_createOptions  create_opts = MQTTAsync_createOptions_initializer5;

    MQTTProperties_add(&props, &property);

    if (!name || !broker_uri || !client_id) {
        return NULL;
    }

    char *server_control_topic    = calloc(1, 128);
    char *server_presence_topic   = calloc(1, 128);
    char *server_capability_topic = calloc(1, 128);

    snprintf(server_control_topic, 128, "$mcp-server/%s/%s", client_id, name);
    snprintf(server_presence_topic, 128, "$mcp-server/presence/%s/%s",
             client_id, name);
    snprintf(server_capability_topic, 128, "$mcp-server/capability/%s/%s",
             client_id, name);

    mcp_server_t *server = calloc(1, sizeof(mcp_server_t));

    server->name = strdup(name);
    if (description) {
        server->description = strdup(description);
    } else {
        server->description = NULL;
    }
    server->broker_uri = strdup(broker_uri);
    server->client_id  = strdup(client_id);
    if (user) {
        server->user = strdup(user);
    } else {
        server->user = NULL;
    }
    if (password) {
        server->password = strdup(password);
    } else {
        server->password = NULL;
    }
    if (cert) {
        server->cert = strdup(cert);
    } else {
        server->cert = NULL;
    }

    int ret = MQTTAsync_createWithOptions(
        &server->client, server->broker_uri, server->client_id,
        MQTTCLIENT_PERSISTENCE_NONE, NULL, &create_opts);
    if (ret != MQTTASYNC_SUCCESS) {
        printf("Failed to create MQTT client, return code %d\n", ret);
        return NULL;
    }
    MQTTAsync_setCallbacks(server->client, server, conn_lost, msg_arrvd, NULL);

    will_opts.topicName = server_presence_topic;
    will_opts.message   = "";
    server->will_opts   = will_opts;

    conn_opts.connectProperties = &props;
    conn_opts.will              = &server->will_opts;
    conn_opts.onSuccess5        = onConnect;
    conn_opts.onFailure5        = onConnectFailure;
    conn_opts.context           = server;

    server->conn_opts = conn_opts;

    server->control_topic    = server_control_topic;
    server->presence_topic   = server_presence_topic;
    server->capability_topic = server_capability_topic;

    return server;
}

void mcp_server_close(mcp_server_t *server)
{
    if (server) {
        free(server->name);
        free(server->broker_uri);
        free(server->client_id);

        free(server->control_topic);
        free(server->presence_topic);
        free(server->capability_topic);

        if (server->description) {
            free(server->description);
        }
        if (server->user) {
            free(server->user);
        }
        if (server->password) {
            free(server->password);
        }
        if (server->cert) {
            free(server->cert);
        }
        for (int i = 0; i < server->n_tools; i++) {
            free(server->tools[i].name);
            if (server->tools[i].description) {
                free(server->tools[i].description);
            }
            for (int j = 0; j < server->tools[i].property_count; j++) {
                free(server->tools[i].properties[j].name);
                if (server->tools[i].properties[j].description) {
                    free(server->tools[i].properties[j].description);
                }
            }
            free(server->tools[i].properties);
        }
        free(server->tools);

        for (int i = 0; i < server->n_resources; i++) {
            free(server->resources[i].uri);
            free(server->resources[i].name);
            if (server->resources[i].description) {
                free(server->resources[i].description);
            }
            if (server->resources[i].mime_type) {
                free(server->resources[i].mime_type);
            }
            if (server->resources[i].title) {
                free(server->resources[i].title);
            }
        }
        free(server->resources);

        if (server->n_clients > 0) {
            for (int i = 0; i < server->n_clients; i++) {
                free(server->clients[i].client_id);
            }
            free(server->clients);
        }
        free(server);
    }
}

int mcp_server_register_tool(mcp_server_t *server, int n_tools,
                             mcp_tool_t *tools)
{
    server->n_tools = n_tools;
    server->tools   = calloc(n_tools, sizeof(mcp_tool_t));

    for (int i = 0; i < n_tools; i++) {
        server->tools[i].name = strdup(tools[i].name);
        server->tools[i].description =
            tools[i].description ? strdup(tools[i].description) : NULL;
        server->tools[i].property_count = tools[i].property_count;
        server->tools[i].properties =
            calloc(tools[i].property_count, sizeof(property_t));
        for (int j = 0; j < tools[i].property_count; j++) {
            server->tools[i].properties[j] = tools[i].properties[j];
        }
        server->tools[i].call = tools[i].call;
    }

    return 0;
}

int mcp_server_register_resources(mcp_server_t *server, int n_resources,
                                  mcp_resource_t   *resources,
                                  mcp_resource_read read_callback)
{
    server->n_resources = n_resources;
    server->resources   = calloc(n_resources, sizeof(mcp_resource_t));

    for (int i = 0; i < n_resources; i++) {
        server->resources[i].uri  = strdup(resources[i].uri);
        server->resources[i].name = strdup(resources[i].name);
        server->resources[i].description =
            resources[i].description ? strdup(resources[i].description) : NULL;
        server->resources[i].mime_type =
            resources[i].mime_type ? strdup(resources[i].mime_type) : NULL;
        server->resources[i].title =
            resources[i].title ? strdup(resources[i].title) : NULL;
    }

    server->read_callback = read_callback;
    return 0;
}

static char *get_user_property(const MQTTProperties *props, const char *key)
{
    static char value[256] = { 0 };
    for (int i = 0; i < props->count; i++) {
        if (props->array[i].identifier == MQTTPROPERTY_CODE_USER_PROPERTY) {
            if (strcmp(props->array[i].value.data.data, key) == 0) {
                strncpy(value, props->array[i].value.value.data,
                        props->array[i].value.value.len);
                return value;
            }
        }
    }
    return NULL;
}

static bool insert_client(mcp_server_t *server, const char *client_id)
{
    for (int i = 0; i < server->n_clients; i++) {
        if (strcmp(server->clients[i].client_id, client_id) == 0) {
            // client already exists
            return false;
        }
    }

    if (server->n_clients == 0) {
        server->clients = calloc(1, sizeof(connect_mcp_client));
    } else {
        server->clients =
            realloc(server->clients,
                    (server->n_clients + 1) * sizeof(connect_mcp_client));
    }
    server->clients[server->n_clients].client_id = strdup(client_id);
    server->n_clients++;
    return true;
}

static bool remove_client(mcp_server_t *server, const char *topic_client_id)
{
    for (int i = 0; i < server->n_clients; i++) {
        char *find = strstr(topic_client_id, server->clients[i].client_id);
        if (find != NULL &&
            strlen(find) == strlen(server->clients[i].client_id)) {
            free(server->clients[i].client_id);
            for (int j = i; j < server->n_clients - 1; j++) {
                server->clients[j] = server->clients[j + 1];
            }
            server->n_clients--;
            if (server->n_clients == 0) {
                free(server->clients);
                server->clients = NULL;
            } else {
                server->clients =
                    realloc(server->clients,
                            server->n_clients * sizeof(connect_mcp_client));
            }
            return true;
        }
    }
    return false;
}

static bool is_client_init(mcp_server_t *server, const char *topic_client)
{
    for (int i = 0; i < server->n_clients; i++) {
        char *find = strstr(topic_client, server->clients[i].client_id);
        if (find != NULL &&
            strlen(find) == strlen(server->clients[i].client_id)) {
            return true; // client is initialized
        }
    }
    return false;
}

static mcp_resource_t *get_resource_by_uri(mcp_server_t *server,
                                           const char   *uri)
{
    for (int i = 0; i < server->n_resources; i++) {
        if (strcmp(server->resources[i].uri, uri) == 0) {
            return &server->resources[i];
        }
    }
    return NULL;
}

static bool mcp_server_tool_check(mcp_server_t *server, const char *tool_name,
                                  int n_args, property_t *args,
                                  mcp_tool_t **tool)
{
    if (server == NULL || tool_name == NULL || args == NULL || n_args <= 0) {
        return false; // Invalid parameters
    }

    for (int i = 0; i < server->n_tools; i++) {
        if (strcmp(server->tools[i].name, tool_name) == 0) {
            if (server->tools[i].property_count != n_args) {
                return false;
            }
            for (int j = 0; j < n_args; j++) {
                if (strcmp(server->tools[i].properties[j].name, args[j].name) !=
                    0) {
                    return false; // Argument name mismatch
                } else {
                    if (server->tools[i].properties[j].type ==
                        PROPERTY_INTEGER) {
                        args[j].type = PROPERTY_INTEGER;
                        args[j].value.integer_value =
                            (long long) args[j].value.real_value;
                    }
                }
            }
            *tool = &server->tools[i];
            return true;
        }
    }
    return false;
}

int msg_arrvd(void *ctx, char *topic, int topicLen, MQTTAsync_message *message)
{
    mcp_server_t *server = (mcp_server_t *) ctx;
    printf("Message arrived on topic: %s %d, %d\n", topic, topicLen,
           message->payloadlen);

    jsonrpc_t *jsonrpc = jsonrpc_decode(message->payload);
    if (jsonrpc == NULL) {
        MQTTAsync_freeMessage(&message);
        MQTTAsync_free(topic);
        return 1;
    }

    char               *method = jsonrpc_get_method(jsonrpc);
    const jsonrpc_id_t *id     = jsonrpc_get_id(jsonrpc);
    if (method == NULL) {
        jsonrpc_decode_free(jsonrpc);
        MQTTAsync_freeMessage(&message);
        MQTTAsync_free(topic);
        return 1;
    }

    printf("Method: %s\n", method);
    if (strncmp(topic, server->control_topic, strlen(server->control_topic)) ==
        0) {
        if (strcmp(method, "initialize") != 0) {
            jsonrpc_decode_free(jsonrpc);
            MQTTAsync_freeMessage(&message);
            MQTTAsync_free(topic);
            return 1;
        }

        if (!jsonrpc_id_exists(id)) {
            jsonrpc_decode_free(jsonrpc);
            MQTTAsync_freeMessage(&message);
            MQTTAsync_free(topic);
            return 1;
        }

        char *client_id =
            get_user_property(&message->properties, "MCP-MQTT-CLIENT-ID");
        if (client_id == NULL) {
            jsonrpc_decode_free(jsonrpc);
            MQTTAsync_freeMessage(&message);
            MQTTAsync_free(topic);
            return 1;
        }

        char *sub_topic = calloc(1, 128);
        snprintf(sub_topic, 128, "$mcp-rpc/%s/%s/%s", client_id,
                 server->client_id, server->name);
        if (insert_client(server, client_id)) {
            MQTTAsync_responseOptions opts =
                MQTTAsync_responseOptions_initializer;
            opts.subscribeOptions.noLocal = 1;
            MQTTAsync_subscribe(server->client, sub_topic, 0, &opts);
        }

        char *response = jsonrpc_encode(jsonrpc_init_response(
            id, server->n_tools > 0, server->n_resources > 0));

        MQTTAsync_message msg = MQTTAsync_message_initializer;
        msg.payload           = (void *) response;
        msg.payloadlen        = (int) strlen(response);
        msg.qos               = 0;
        msg.retained          = 0;

        MQTTAsync_sendMessage(server->client, sub_topic, &msg, NULL);
        free(response);
        free(sub_topic);
    }

    if (strncmp(topic, "$mcp-client/presence/",
                strlen("$mcp-client/presence/")) == 0) {
        if (message->payloadlen == 0) {
            char *client_topic = calloc(1, topicLen + 1);
            strncpy(client_topic, topic, topicLen);
            remove_client(server, client_topic);
            free(client_topic);
        }
    }

    if (strncmp(topic, "$mcp-rpc/", strlen("$mcp-rpc/")) == 0) {
        if (strcmp(method, "notifications/initialized") == 0) {
            // client initialized
        }
        char *response = NULL;
        if (strcmp(method, "tools/list") == 0) {
            response = jsonrpc_encode(
                jsonrpc_tool_list_response(id, server->n_tools, server->tools));
        }
        if (strcmp(method, "tools/call") == 0) {
            char       *f_name = NULL;
            int         n_args = 0;
            mcp_tool_t *tool   = NULL;
            property_t *args   = NULL;

            int ret =
                jsonrpc_tool_call_decode(jsonrpc, &f_name, &n_args, &args);
            if (ret != 0) {
                response = jsonrpc_encode(
                    jsonrpc_error_response(id, -32600, "Invalid params"));
            } else {
                if (mcp_server_tool_check(server, f_name, n_args, args,
                                          &tool)) {
                    const char *result = tool->call(n_args, args);
                    response =
                        jsonrpc_encode(jsonrpc_tool_call_response(id, result));
                } else {
                    response = jsonrpc_encode(
                        jsonrpc_error_response(id, -32601, "Method not found"));
                }

                for (int i = 0; i < server->n_tools; i++) {
                    if (args[i].type == PROPERTY_STRING) {
                        free(args[i].value.string_value);
                    }
                }
                free(args);
            }
        }
        if (strcmp(method, "resources/list") == 0) {
            response = jsonrpc_encode(jsonrpc_resource_list_response(
                id, server->n_resources, server->resources));
        }
        if (strcmp(method, "resources/read") == 0) {
            char *uri = NULL;
            int   ret = jsonrpc_resource_read_decode(jsonrpc, &uri);

            if (ret == 0) {
                mcp_resource_t *resource = get_resource_by_uri(server, uri);
                if (resource) {
                    const char *content = server->read_callback(uri);
                    response =
                        jsonrpc_encode(jsonrpc_resource_read_text_response(
                            id, resource, content));
                }

                free(uri);
            }
        }

        if (response) {
            MQTTAsync_message msg = MQTTAsync_message_initializer;
            msg.payload           = (void *) response;
            msg.payloadlen        = (int) strlen(response);
            msg.qos               = 0;
            msg.retained          = 0;
            int rr = MQTTAsync_sendMessage(server->client, topic, &msg, NULL);
            printf("Sending response to topic: %d %s\n %s\n", rr, topic,
                   response);
            free(response);
        }
    }

    jsonrpc_decode_free(jsonrpc);
    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topic);

    return 1;
}

int mcp_server_run(mcp_server_t *server)
{
    int ret = MQTTAsync_connect(server->client, &server->conn_opts);
    printf("Connecting to MQTT broker: %s\n", server->broker_uri);
    return ret;
}