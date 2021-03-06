/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */

#pragma once

#include <fc/filesystem.hpp>

#include <cstdlib>


namespace graphene { namespace utilities {
   
   
   class decent_path_finder {
   private:
      // Constructor may throw exceptions. Which is bad in general, but if it fails program execution must be terminated
      decent_path_finder();
      decent_path_finder(const decent_path_finder&) {}
      
   public:
      static decent_path_finder& instance() {
         static decent_path_finder theChoosenOne;
         return theChoosenOne;
      }
      
   public:
      fc::path get_user_home()   const { return _user_home; }

      fc::path get_decent_home() const { return _decent_home; }
      fc::path get_decent_data() const { return _decent_data; }
      fc::path get_decent_packages() const {
         if( _packages_path != fc::path() ){
            return _packages_path;
         }
         return _decent_data / "packages";
      }
      fc::path get_decent_logs() const { return _decent_logs; }
      fc::path get_decent_temp() const { return _decent_temp; }
      fc::path get_ipfs_bin() const { return _ipfs_bin; }
      fc::path get_ipfs_path() const { return _ipfs_path; }

      void set_decent_data_path(const fc::path& path) { _decent_data = path; }
      void set_decent_logs_path(const fc::path& path) { _decent_logs = path; }
      void set_decent_temp_path(const fc::path& path) { _decent_temp = path; }
      void set_packages_path(const fc::path& packages_path) { _packages_path = packages_path; }
      
   private:
      fc::path _user_home;
      fc::path _decent_home;
      fc::path _decent_data;
      fc::path _decent_logs;
      fc::path _decent_temp;
      fc::path _ipfs_bin;
      fc::path _ipfs_path;
      fc::path _packages_path;
   };
   
   
} } // graphene::utilities
