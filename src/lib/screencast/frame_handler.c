#include "../../../include/utils.h"
#include "../../../include/screencast.h"
#include <pipewire/pipewire.h>
// #include <pipewire/pipewire-glib.h>

// int _list_existing_nodes(struct pw_core * core) {
//     struct pw_registry *registry;
//     struct pw_registry_events registry_events = {0};

//     // Create a registry to list nodes
//     registry = pw_core_get_registry(core, PW_KEY_INTERFACE, 0);

//     // Set up listener for registry events
//     registry_events.node_added = on_node_added;
//     pw_registry_add_listener(registry, &registry_events, NULL);

//     // Trigger scanning of existing nodes
//     pw_registry_scan(registry);
// }

static void registry_event_global(void *data, uint32_t id,
                uint32_t permissions, const char *type, uint32_t version,
                const struct spa_dict *props)
{
        printf("object: id:%u type:%s/%d\n", id, type, version);
}

static const struct pw_registry_events registry_events = {
        PW_VERSION_REGISTRY_EVENTS,
        .global = registry_event_global,
};

void _setup_pipewire(_DbusConnectionHandle *conn) {
    struct pw_main_loop *loop;
    struct pw_context *context;
    struct pw_core *core;
    struct pw_registry *registry;
    struct spa_hook registry_listener;

    // Create PipeWire context and core
    loop = pw_main_loop_new(NULL);
    context = pw_context_new(pw_main_loop_get_loop(loop), NULL, 0);
    core = pw_context_connect_fd(context, conn->pipewire_fd, NULL, 0);

    registry = pw_core_get_registry(core, PW_VERSION_REGISTRY, 0);
    spa_zero(registry_listener);
    pw_registry_add_listener(registry, &registry_listener,
                                   &registry_events, NULL);
    pw_main_loop_run(loop);

    pw_proxy_destroy((struct pw_proxy*)registry);
    pw_core_disconnect(core);
    pw_context_destroy(context);
    pw_main_loop_destroy(loop);

}
