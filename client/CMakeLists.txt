cmake_minimum_required(VERSION 3.15)
project(CubeCraft3)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(dep)
add_subdirectory(shader)
if (NOT TARGET CubeCraft3_resource)
    add_subdirectory(../resource ../resource)
endif ()
if (NOT TARGET CubeCraft3_common)
    add_subdirectory(../common ../common)
endif ()

add_executable(CubeCraft3_client
        src/main.cpp
        include/client/Config.hpp
        src/Application.cpp
        include/client/Application.hpp
        src/Chunk.cpp
        include/client/Chunk.hpp
        src/World.cpp
        include/client/World.hpp
        src/ChunkWorkerBase.cpp
        include/client/ChunkWorkerBase.hpp
        src/ChunkMesher.cpp
        include/client/ChunkMesher.hpp
        src/WorldRenderer.cpp
        include/client/WorldRenderer.hpp
        src/Camera.cpp
        include/client/Camera.hpp
        src/GlobalTexture.cpp
        include/client/GlobalTexture.hpp
        src/ENetClient.cpp
        include/client/ENetClient.hpp
        include/client/ClientBase.hpp
        src/LocalClient.cpp
        include/client/LocalClient.hpp
        src/ChunkGenerator.cpp
        include/client/ChunkGenerator.hpp
        include/client/ChunkMesh.hpp
        include/client/TerrainBase.hpp
        src/DefaultTerrain.cpp
        include/client/DefaultTerrain.hpp
        include/client/MeshCluster.hpp
        include/client/MeshHandle.hpp
        include/client/MeshRendererBase.hpp
        src/ChunkRenderer.cpp
        include/client/ChunkRenderer.hpp
        include/client/WorkerBase.hpp
        include/client/MeshEraser.hpp
        src/DepthHierarchy.cpp
        include/client/DepthHierarchy.hpp
        src/ChunkLighter.cpp
        include/client/ChunkLighter.hpp
        src/OITCompositor.cpp
        include/client/OITCompositor.hpp
        src/PostProcessor.cpp
        include/client/PostProcessor.hpp
        src/ScreenRenderer.cpp
        include/client/ScreenRenderer.hpp)

target_link_libraries(CubeCraft3_client PRIVATE client_dep client_shader CubeCraft3_resource CubeCraft3_common)
target_include_directories(CubeCraft3_client PRIVATE include)
