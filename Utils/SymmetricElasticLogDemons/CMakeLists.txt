cmake_minimum_required(VERSION 2.4)
if(COMMAND cmake_policy)
  cmake_policy(VERSION 2.4)
  cmake_policy(SET CMP0005 OLD)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

find_package(ITK REQUIRED)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
)
add_executable(elasticlogdomaindemonsregistration ElasticLogDomainDemonsRegistration.cxx )
target_link_libraries(elasticlogdomaindemonsregistration ${ITK_LIBRARIES} )


if(NOT ${PROJECT_NAME}_INSTALL_NO_RUNTIME)
  install(TARGETS elasticlogdomaindemonsregistration
    BUNDLE DESTINATION bin
    RUNTIME DESTINATION bin
    )
  install_files(/include "(\\.h|\\.txx)$")
endif()

