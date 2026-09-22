#include "SFML attempt/flecs.h"
struct ReligionType {};
void test(flecs::world& world) {
    auto q = world.query_builder().with<ReligionType>().build();
    q.each([](flecs::entity e) {});
}
