#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/mcp.h"
#include "../include/mcp_server.h"

void signal_handler(int sig)
{
    (void) sig;
}

const char *add_numbers(int n_args, property_t *args)
{
    (void) n_args;
    int a = args[0].value.integer_value;
    int b = args[1].value.integer_value;

    int result = a + b;

    char *response = malloc(32);
    snprintf(response, 32, "%d", result);
    return response;
}

mcp_tool_t tool = {
    .name           = "add",
    .description    = "Adds two numbers",
    .call           = add_numbers,
    .property_count = 2,
    .properties =
        (property_t[]) {
            {
                .name                = "a",
                .description         = "First number",
                .type                = PROPERTY_INTEGER,
                .value.integer_value = 0,
            },
            {
                .name                = "b",
                .description         = "Second number",
                .type                = PROPERTY_INTEGER,
                .value.integer_value = 0,
            },
        },
};

void mcp_server_example()
{
    mcp_server_t *server = mcp_server_init(
        "ESP32 Demo Server Name", "This is an example MCP server",
        "tcp://broker.emqx.io:1883", "example_client", NULL, NULL, NULL);

    mcp_server_register_tool(server, 1, &tool);

    mcp_server_run(server);
}

int main()
{
    mcp_server_example();

    while (1) {
        sleep(1);
    }

    return 0;
}
