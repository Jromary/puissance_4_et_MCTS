cmake_minimum_required(VERSION 3.10)
project(puissance_4_et_MCTS C)

set(CMAKE_C_STANDARD 11)
add_compile_options(-O3)

add_executable(puissance_4_et_MCTS
        src/jeu.c)
target_link_libraries(puissance_4_et_MCTS m)
