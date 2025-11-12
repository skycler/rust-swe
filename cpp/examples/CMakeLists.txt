cmake_minimum_required(VERSION 3.20)

# Dam break example
add_executable(dam_break dam_break.cpp)
target_link_libraries(dam_break PRIVATE swe_lib)

# Radial dam break example
add_executable(radial_dam radial_dam.cpp)
target_link_libraries(radial_dam PRIVATE swe_lib)

# Tsunami example
add_executable(tsunami tsunami.cpp)
target_link_libraries(tsunami PRIVATE swe_lib)
