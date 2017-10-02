/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */

#include "ipfs_transfer.hpp"
#include "ipfs_wrapper.hpp"

#include <decent/package/package.hpp>
#include <decent/package/package_config.hpp>

#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <json.hpp>

#include <vector>
#include <string>
#include <regex>

#include <pwd.h>

namespace decent { namespace package {


    namespace detail {


        bool parse_ipfs_url(const std::string& url, std::string& obj_id) {
            const std::string ipfs = "ipfs:";
            if (url.substr(0, ipfs.size()) == ipfs) {
                obj_id = url.substr(ipfs.size());
                boost::algorithm::trim_left_if(obj_id, [](char ch) { return ch == '/'; });
                boost::algorithm::trim_right_if(obj_id, [](char ch) { return ch == '/'; });
                return true;
            }
            return false;
        }

        std::string get_ipfs_repo_path()
        {
            std::string repo_path(graphene::utilities::decent_path_finder::instance().get_user_home().string());
            repo_path += "/.ipfs";
            return repo_path;
        }


    } // namespace detail


    // _client(PackageManagerConfigurator::instance().get_ipfs_host(), PackageManagerConfigurator::instance().get_ipfs_port())

    IPFSDownloadPackageTask::IPFSDownloadPackageTask(PackageInfo& package)
        : detail::PackageTask(package),
          m_ipfs(IpfsWrapper::instance())
    {
        if (!m_ipfs.IsStarted()) {

            if (!m_ipfs.Initialize(detail::get_ipfs_repo_path().c_str() )) {
                throw std::runtime_error("ipfs could not initialized!");
            }
        }
    }

    uint64_t IPFSDownloadPackageTask::ipfs_recursive_get_size(const std::string &url)
    {
        uint64_t size = 0;

        std::string result, tmp_url;
        std::stack<std::string> stack;
        stack.push(url);

        while(!stack.empty()) {
            tmp_url = stack.top();
            stack.pop();

            bool ret = m_ipfs.ipfs_ls(url, result);
            if(!ret) {
                //TODO: error handling...
                return 0;
            }

            nlohmann::json object = nlohmann::json::parse(result);
            nlohmann::json links = object.at("Links");

            for (auto link : links) {

                if((int) link.at("Type") == 1 ) //directory
                {
                    tmp_url = link.at("Hash");
                    stack.push(tmp_url);
                }
                else if((int) link.at("Type") == 2 ) //file
                {
                    size += (uint64_t) link.at("Size");
                }
            }
        }

        return size;
    }

    void IPFSDownloadPackageTask::ipfs_recursive_get(const std::string &url,
                                                         const boost::filesystem::path &dest_path)
    {
        ilog("ipfs_recursive_get called for url ${u}",("u", url));
        FC_ASSERT( exists(dest_path) && is_directory(dest_path) );

        std::string result;
        bool ret = m_ipfs.ipfs_ls(url, result);
        if (!ret) {
            throw std::runtime_error("package not found!");
        }

        nlohmann::json object = nlohmann::json::parse(result);
        nlohmann::json links = object.at("Links");

        for( auto &link : links ) {
            if((int) link.at("Type") == 1 ) //directory
            {
                const auto dir_name = dest_path / link.at("Name");
                create_directories(dir_name);
                ipfs_recursive_get(link.at("Hash"), dir_name);
            }
            else if((int) link.at("Type") == 2 ) //file
            {
                uint64_t size = (uint64_t) link.at("Size");
                const std::string file_name = link.at("Name");
                const std::string file_obj_id = link.at("Hash");

                std::string filename = (dest_path / file_name).string();

                ret = m_ipfs.ipfs_get(file_obj_id, filename);
                if (!ret) {
                    //TODO: error....
                }

                _package._downloaded_size += size;
            }
            PACKAGE_TASK_EXIT_IF_REQUESTED;
        }

    }

    void IPFSDownloadPackageTask::task() {
        PACKAGE_INFO_GENERATE_EVENT(package_download_start, ( ) );

        using namespace boost::filesystem;

        const auto temp_dir_path = unique_path(graphene::utilities::decent_path_finder::instance().get_decent_temp() / "%%%%-%%%%-%%%%-%%%%");

        try {
            PACKAGE_TASK_EXIT_IF_REQUESTED;

            std::string obj_id;

            if (!detail::parse_ipfs_url(_package._url, obj_id)) {
                FC_THROW("'${url}' is not an ipfs NURI", ("url", _package._url));
            }

            create_directories(temp_dir_path);
            remove_all(temp_dir_path);
            create_directories(temp_dir_path);

            PACKAGE_INFO_CHANGE_TRANSFER_STATE(DOWNLOADING);

            _package._size = ipfs_recursive_get_size( obj_id );
            _package._downloaded_size = 0;
            
            ipfs_recursive_get( obj_id, temp_dir_path );

            const auto content_file = temp_dir_path / "content.zip.aes";

            _package._hash = detail::calculate_hash(content_file);
            const auto package_dir = _package.get_package_dir();

            PACKAGE_TASK_EXIT_IF_REQUESTED;

            _package.lock_dir();

            PACKAGE_INFO_CHANGE_DATA_STATE(PARTIAL);

            std::set<boost::filesystem::path> paths_to_skip;

            paths_to_skip.clear();
            paths_to_skip.insert(_package.get_lock_file_path());
            detail::remove_all_except(package_dir, paths_to_skip);

            PACKAGE_TASK_EXIT_IF_REQUESTED;

            paths_to_skip.clear();
            paths_to_skip.insert(_package.get_package_state_dir(temp_dir_path));
            paths_to_skip.insert(_package.get_lock_file_path(temp_dir_path));
            detail::move_all_except(temp_dir_path, package_dir, paths_to_skip);

            remove_all(temp_dir_path);

            PACKAGE_INFO_CHANGE_DATA_STATE(CHECKED);
            PACKAGE_INFO_CHANGE_TRANSFER_STATE(TS_IDLE);
            PACKAGE_INFO_GENERATE_EVENT(package_download_complete, ( ) );
        }
        catch ( const fc::exception& ex ) {
            remove_all(temp_dir_path);
            _package.unlock_dir();
            PACKAGE_INFO_CHANGE_DATA_STATE(INVALID);
            PACKAGE_INFO_CHANGE_TRANSFER_STATE(TS_IDLE);
            PACKAGE_INFO_GENERATE_EVENT(package_download_error, ( ex.to_detail_string() ) );
            throw;
        }
        catch ( const std::exception& ex ) {
            remove_all(temp_dir_path);
            _package.unlock_dir();
            PACKAGE_INFO_CHANGE_DATA_STATE(INVALID);
            PACKAGE_INFO_CHANGE_TRANSFER_STATE(TS_IDLE);
            PACKAGE_INFO_GENERATE_EVENT(package_download_error, ( ex.what() ) );
            throw;
        }
        catch ( ... ) {
            remove_all(temp_dir_path);
            _package.unlock_dir();
            PACKAGE_INFO_CHANGE_DATA_STATE(INVALID);
            PACKAGE_INFO_CHANGE_TRANSFER_STATE(TS_IDLE);
            PACKAGE_INFO_GENERATE_EVENT(package_download_error, ( "unknown" ) );
            throw;
        }
    }

    IPFSStartSeedingPackageTask::IPFSStartSeedingPackageTask(PackageInfo& package)
        : detail::PackageTask(package),
          m_ipfs(IpfsWrapper::instance())
    {
        if (!m_ipfs.IsStarted()) {
            if (!m_ipfs.Initialize(detail::get_ipfs_repo_path().c_str() )) {
                throw std::runtime_error("ipfs could not initialized!");
            }
        }
    }

    void IPFSStartSeedingPackageTask::task() {
        PACKAGE_INFO_GENERATE_EVENT(package_seed_start, ( ) );

        try {
            PACKAGE_TASK_EXIT_IF_REQUESTED;
#if 0
            std::set<boost::filesystem::path> paths_to_skip;
            paths_to_skip.insert(_package.get_package_state_dir());
            paths_to_skip.insert(_package.get_lock_file_path());

            std::vector<boost::filesystem::path> files;
            detail::get_files_recursive_except(_package.get_package_dir(), files, paths_to_skip);

            std::vector<ipfs::http::FileUpload> files_to_add;

            const auto package_base_path = _package._parent_dir.lexically_normal();

            for (auto& file : files) {
#ifdef _MSC_VER 
                std::string fileUnixPathDelim = file.string();
                std::string::iterator iter = fileUnixPathDelim.begin();
                while (iter != fileUnixPathDelim.end())
                {
                   if ((*iter) == '\\')
                      (*iter) = '/';
                   iter++;
                }
 
                const auto file_rel_path = detail::get_relative(package_base_path, file.lexically_normal());
                
                std::string file_relPath_UnixPathDelim = file_rel_path.string();
                iter = file_relPath_UnixPathDelim.begin();
                while (iter != file_relPath_UnixPathDelim.end())
                {
                   if ((*iter) == '\\')
                      (*iter) = '/';
                   iter++;
                }
                files_to_add.push_back({ file_relPath_UnixPathDelim, ipfs::http::FileUpload::Type::kFileName, fileUnixPathDelim });
#else
                const auto file_rel_path = detail::get_relative(package_base_path, file.lexically_normal());
               files_to_add.push_back({ file_rel_path.string(), ipfs::http::FileUpload::Type::kFileName, file.string() });
#endif
            }
#endif //0


            PACKAGE_INFO_CHANGE_TRANSFER_STATE(SEEDING);

#if 0

            ipfs::Json added_files;
            _client.FilesAdd(files_to_add, &added_files);

//          PACKAGE_INFO_GENERATE_EVENT(package_seed_progress, ( ) );

            std::string root_hash;
            for (auto& added_file : added_files) {
                if (added_file.at("path") == _package._hash.str()) {
                    root_hash = added_file.at("hash");
                    break;
                }
            }

            if (root_hash.empty()) {
                FC_THROW("Unable to find root hash in 'ipfs add' results");
            }

            _package._url = "ipfs:" + root_hash;

            _client.PinAdd(root_hash); // just in case

            // TODO: remove the actual files?

#endif

            PACKAGE_INFO_CHANGE_TRANSFER_STATE(SEEDING);
            PACKAGE_INFO_GENERATE_EVENT(package_seed_complete, ( ) );
        }
        catch ( const fc::exception& ex ) {
            PACKAGE_INFO_CHANGE_TRANSFER_STATE(TS_IDLE);
            PACKAGE_INFO_GENERATE_EVENT(package_seed_error, ( ex.to_detail_string() ) );
            throw;
        }
        catch ( const std::exception& ex ) {
            PACKAGE_INFO_CHANGE_TRANSFER_STATE(TS_IDLE);
            PACKAGE_INFO_GENERATE_EVENT(package_seed_error, ( ex.what() ) );
            throw;
        }
        catch ( ... ) {
            PACKAGE_INFO_CHANGE_TRANSFER_STATE(TS_IDLE);
            PACKAGE_INFO_GENERATE_EVENT(package_seed_error, ( "unknown" ) );
            throw;
        }
    }

    IPFSStopSeedingPackageTask::IPFSStopSeedingPackageTask(PackageInfo& package)
        : detail::PackageTask(package),
          m_ipfs(IpfsWrapper::instance())
    {
        if (!m_ipfs.IsStarted()) {
            if (!m_ipfs.Initialize(detail::get_ipfs_repo_path().c_str()  )) {
                throw std::runtime_error("ipfs could not initialized!");
            }
        }
    }

    void IPFSStopSeedingPackageTask::task() {
        try {
            PACKAGE_TASK_EXIT_IF_REQUESTED;

            std::string obj_id;

            if (!detail::parse_ipfs_url(_package._url, obj_id)) {
                FC_THROW("'${url}' is not an ipfs NURI", ("url", _package._url));
            }

            m_ipfs.ipfs_pin_rem(obj_id, true);

            PACKAGE_INFO_CHANGE_TRANSFER_STATE(TS_IDLE);
        }
        catch ( const fc::exception& ex ) {
            PACKAGE_INFO_CHANGE_TRANSFER_STATE(TS_IDLE);
            PACKAGE_INFO_GENERATE_EVENT(package_seed_error, ( ex.to_detail_string() ) );
            throw;
        }
        catch ( const std::exception& ex ) {
            PACKAGE_INFO_CHANGE_TRANSFER_STATE(TS_IDLE);
            PACKAGE_INFO_GENERATE_EVENT(package_seed_error, ( ex.what() ) );
            throw;
        }
        catch ( ... ) {
            PACKAGE_INFO_CHANGE_TRANSFER_STATE(TS_IDLE);
            PACKAGE_INFO_GENERATE_EVENT(package_seed_error, ( "unknown" ) );
            throw;
        }
    }

    std::shared_ptr<detail::PackageTask> IPFSTransferEngine::create_download_task(PackageInfo& package) {
        return std::make_shared<IPFSDownloadPackageTask>(package);
    }

    std::shared_ptr<detail::PackageTask> IPFSTransferEngine::create_start_seeding_task(PackageInfo& package) {
        return std::make_shared<IPFSStartSeedingPackageTask>(package);
    }

    std::shared_ptr<detail::PackageTask> IPFSTransferEngine::create_stop_seeding_task(PackageInfo& package) {
        return std::make_shared<IPFSStopSeedingPackageTask>(package);
    }


} } // namespace decent::package


