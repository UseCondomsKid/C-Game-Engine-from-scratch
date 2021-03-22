//This ECS is taken from https://www.youtube.com/watch?v=s6TMa33niJo&t=55s
//It is not final and it is bound to change


#ifndef ECS_H_INCLUDED
#define ECS_H_INCLUDED

#include <inttypes.h>
#include <stddef.h>

#define ENTITY_FLAG_ALIVE 1

typedef struct 
{
	uint32_t id;

} Entity;

typedef struct 
{
	uint32_t count;
	uint32_t cap;
	uint32_t *list;

} QueryResult;

/**
 * Initialise the ECS module with the component sizes.
 * 
 * Must pass in the size of each component type in the order you want
 * to store them. The maximum number of component types is 32, though
 * this could be extended by adding another bitmask and a bit switch.
 */
void Ecs_init(uint32_t component_count, ...);

/**
 * Create an entity. Returns a handle which contains the id.
 */
Entity Ecs_create();

/**
 * Get component data from an entity.
 */
void *Ecs_get(uint32_t entity_id, uint32_t component_id);

/**
 * Add a component with data to an entity.
 */
void Ecs_add(uint32_t entity_id, uint32_t component_id, void *data);

/**
 * Remove a component from an entity.
 */
void Ecs_remove(uint32_t entity_id, uint32_t component_id);

/**
 * Returns true if entity has component.
 */
uint32_t Ecs_has(uint32_t entity_id, uint32_t component_id);

/**
 * Kill an entity.
 */
void Ecs_kill(uint32_t entity_id);

/**
 * Find all entities which have components with IDs.
 */
QueryResult *Ecs_query(uint32_t component_count, ...);

#endif
