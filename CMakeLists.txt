set(CMAKE_C_COMPILER c99)
cmake_minimum_required(VERSION 2.6)
project(mq-utils)

if(${CMAKE_SYSTEM_NAME} MATCHES "SunOS")
	set(CMAKE_INSTALL_RPATH "/usr/sfw/lib;/usr/ccs/lib")
endif(${CMAKE_SYSTEM_NAME} MATCHES "SunOS")

if ( CMAKE_COMPILER_IS_GNUCC )
	add_compile_options(-Wall)
endif ( CMAKE_COMPILER_IS_GNUCC )


add_compile_options(-g)
add_definitions(-D_GNU_SOURCE -DHAVE_EXPAT_H -D__EXTENSIONS__ -D_XPG6)
include_directories(/usr/include /usr/local/include /opt/mqm/inc include
/usr/sfw/include /usr/include/glib-2.0 /usr/lib64/glib-2.0/include
/usr/lib/glib-2.0/include ~/include)
link_directories(/lib64 /usr/lib64 /usr/sfw/lib /usr/lib /usr/ccs/lib ~/lib)


link_libraries(ssl paho-mqtt3c expat glib-2.0)

add_executable(shpion src/shpion.c)
add_executable(shpion_sub src/shpion_sub.c)

install(TARGETS shpion shpion_sub DESTINATION /usr/local/bin)
