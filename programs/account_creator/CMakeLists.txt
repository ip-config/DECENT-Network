
add_executable( account_creator main.cpp )

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling cli_wallet with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

target_link_libraries( account_creator
        PRIVATE graphene_app graphene_net graphene_chain graphene_egenesis_decent graphene_utilities graphene_wallet package_manager decent_encrypt
        pbc
        fc
        ${CMAKE_DL_LIBS}
        ${PLATFORM_SPECIFIC_LIBS}
)

if( WIN32 )
  target_link_libraries( account_creator PRIVATE ${GMP_LIBRARIES} )
else()
  target_link_libraries( account_creator PRIVATE gmp )
endif()

if(MSVC)
  set_source_files_properties( main.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

#install( TARGETS
#   account_creator
#
#   RUNTIME DESTINATION bin
#   LIBRARY DESTINATION lib
#   ARCHIVE DESTINATION lib
#)
