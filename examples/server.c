#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "mcp/mcp.h"
#include "mcp/server/server.h"

void signal_handler(int sig)
{
    (void) sig;
}

const char *add_numbers(size_t n_args, mcp_tool_arg_t *args)
{
    (void) n_args;
    int a = args[0].value.value.i;
    int b = args[1].value.value.i;

    int result = a + b;

    char *response = malloc(32);
    snprintf(response, 32, "%d", result);
    return response;
}

mcp_tool_t tool = {
    .name        = "add",
    .description = "Adds two numbers",
    .call        = add_numbers,
    .n_args      = 2,
    .args =
        (mcp_tool_arg_t[]) {
            {
                .name        = "a",
                .description = "First number",
                .value       = { .value_e = MCP_VALUE_INT, .value.i = 0 },
            },
            {
                .name        = "b",
                .description = "Second number",
                .value       = { .value_e = MCP_VALUE_INT, .value.i = 0 },
            },
        },
};

mcp_resource_t resources[] = {
    {
        .uri         = "file://resource/",
        .name        = "Example Resource",
        .description = "This is an example resource ",
        .mime_type   = "text/plain",
        .is_binary   = false,
    },
};

int resource_read(const mcp_resource_t *resource, uint8_t **bytes,
                  size_t *n_bytes)
{
    (void) resource;
    const char *data = "This is the content of the example resource.";
    *n_bytes         = strlen(data) + 1;
    *bytes           = (uint8_t *) strdup(data);
    return 0; // Success
}

void mcp_server_example()
{
    mcp_server_tp_t tp     = { .tp_e = MCP_SERVER_STDIO };
    mcp_server_t *  server = mcp_server_init(tp, 4096);

    mcp_server_register_tools(server, &tool, 1);
    mcp_server_register_resources(server, resources, 1, resource_read);

    mcp_server_run(server);

    printf("mcp server example stopped\n");
}

int main()
{
    mcp_server_example();
    return 0;
}
