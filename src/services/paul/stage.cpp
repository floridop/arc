#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/XMLNode.h>
#include <arc/GUID.h>
#include <arc/User.h>
#include <arc/UserConfig.h>
#include <arc/URL.h>
#include <arc/Logger.h>

#include <arc/data/FileCache.h>
#include <arc/data/DataHandle.h>
#include <arc/data/DataMover.h>
#include <arc/data/URLMap.h>

#include "paul.h"

namespace Paul
{

class PointPair 
{
    private:
        Arc::URL source_url;
        Arc::URL destination_url;
    public:
        Arc::DataHandle source;
        Arc::DataHandle destination;
        PointPair(const std::string& source_str,
                  const std::string& destination_str,
                  const Arc::UserConfig& usercfg)
          : source_url(source_str),
            destination_url(destination_str),
            source(source_url, usercfg),
            destination(destination_url, usercfg) {};
        ~PointPair(void) {};
};

class FileTransfer
{
    private:
        Arc::DataMover *mover;
        Arc::FileCache *cache;
        Arc::URLMap url_map;
        Arc::Logger logger_;
        unsigned long long int min_speed;
        time_t min_speed_time;
        unsigned long long int min_average_speed;
        time_t max_inactivity_time;
        std::string cache_path;

    public:
        ~FileTransfer()
        {
            delete mover;
            delete cache;
        };

        FileTransfer(const std::string &_cache_path):logger_(Arc::Logger::rootLogger, "Paul-FileTransfer") {
            cache_path = _cache_path;
            logger_.msg(Arc::VERBOSE, "Filetransfer created");
    
        };

        void create_cache(Job &j)
        {
            // Create cache
            Arc::User cache_user;
            std::string job_id = j.getID();
            std::string cache_dir = cache_path;
#ifndef WIN32
            cache = new Arc::FileCache (cache_dir, job_id, 
#else
            std::string cache_data_dir = cache_dir; 
            std::string cache_link_dir;
            cache = new Arc::FileCache (cache_dir, cache_data_dir,
                                        cache_link_dir, job_id,
#endif
                                        cache_user.get_uid(),
                                        cache_user.get_gid());
        }

        void download(const std::string &job_root, Job &j)
        {
            // Create mover
            mover = new Arc::DataMover();
            mover->retry(true);
            mover->secure(false); // XXX what if I download form https url? 
            mover->passive(false);
            mover->verbose(true);
            min_speed = 0;
            min_speed_time = 300;
            min_average_speed = 0;
            max_inactivity_time = 300;
            if (min_speed != 0) {
                mover->set_default_min_speed(min_speed,min_speed_time);
            }
            if (min_average_speed != 0) {
                mover->set_default_min_average_speed(min_average_speed);
            }
            if(max_inactivity_time != 0) {
                mover->set_default_max_inactivity_time(max_inactivity_time);
            }
            create_cache(j);
            logger_.msg(Arc::VERBOSE, "download");
            Arc::XMLNode jd = j.getJSDL()["JobDescription"];
            std::string xml_str;
            j.getJSDL().GetXML(xml_str);
            logger_.msg(Arc::VERBOSE, xml_str);
            Arc::XMLNode ds;
            for (int i = 0; (ds = jd["DataStaging"][i]) != false; i++) {
                std::string dest = Glib::build_filename(Glib::build_filename(job_root, j.getID()), (std::string)ds["FileName"]);
                Arc::XMLNode s = ds["Source"];
                if (s == false) {
                    // it should not download
                    continue;
                }
                std::string src = (std::string)s["URI"];
                logger_.msg(Arc::VERBOSE, "%s -> %s", src, dest);
    
                std::string failure;
                Arc::UserConfig usercfg(true);
                PointPair *pair = new PointPair(src, dest, usercfg);
                if (pair->source == NULL) {
                    logger_.msg(Arc::ERROR, "Cannot accept source as URL");
                    delete pair;
                    continue;
                }
                if (pair->destination == NULL) {
                    logger_.msg(Arc::ERROR, "Cannot accept destination as URL");
                    delete pair;
                    continue;
                }
                Arc::DataStatus res = mover->Transfer(*(pair->source), *(pair->destination), 
                                                      *cache, url_map, 
                                                      min_speed, min_speed_time, 
                                                      min_average_speed, max_inactivity_time); 
                if (!res.Passed()) {
                    logger_.msg(Arc::ERROR, std::string(res));
                    delete pair;
                    continue;
                }
                delete pair;
            }
        };

        void upload(const std::string &job_root, Job &j)
        {
            // Create mover
            mover = new Arc::DataMover();
            mover->retry(true);
            mover->secure(false); // XXX what if I download form https url? 
            mover->passive(false);
            mover->verbose(true);
            min_speed = 0;
            min_speed_time = 300;
            min_average_speed = 0;
            max_inactivity_time = 300;
            if (min_speed != 0) {
                mover->set_default_min_speed(min_speed,min_speed_time);
            }
            if (min_average_speed != 0) {
                mover->set_default_min_average_speed(min_average_speed);
            }
            if(max_inactivity_time != 0) {
                mover->set_default_max_inactivity_time(max_inactivity_time);
            }
            create_cache(j);
            Arc::XMLNode jd = j.getJSDL()["JobDescription"];
            Arc::XMLNode ds;
            for (int i = 0; (ds = jd["DataStaging"][i]) != false; i++) {
                std::string src = Glib::build_filename(Glib::build_filename(job_root, j.getID()), (std::string)ds["FileName"]);
                Arc::XMLNode d = ds["Target"];
                if (d == false) {
                    // it should not upload
                    continue;
                }
                std::string dest = (std::string)d["URI"];
                logger_.msg(Arc::VERBOSE, "%s -> %s", src, dest);
    
                std::string failure;
                Arc::UserConfig usercfg(true);
                PointPair *pair = new PointPair(src, dest, usercfg);
                if (pair->source == NULL) {
                    logger_.msg(Arc::ERROR, "Cannot accept source as URL");
                    delete pair;
                    continue;
                }
                if (pair->destination == NULL) {
                    logger_.msg(Arc::ERROR, "Cannot accept destination as URL");
                    delete pair;
                    continue;
                }
                Arc::DataStatus res = mover->Transfer(*(pair->source), *(pair->destination), 
                                                      *cache, url_map, 
                                                      min_speed, min_speed_time, 
                                                      min_average_speed, max_inactivity_time);
                if (!res.Passed()) {
                    logger_.msg(Arc::ERROR, std::string(res));
                    delete pair;
                    continue;
                }
                logger_.msg(Arc::VERBOSE, "Transfer completed");
                delete pair;
            }
        };
        
};

bool PaulService::stage_in(Job &j)
{
    logger_.msg(Arc::VERBOSE, "Stage in");
    
    FileTransfer ft(configurator.getCachePath());
    ft.download(configurator.getJobRoot(), j);
    return true;
}

bool PaulService::stage_out(Job &j)
{
    logger_.msg(Arc::VERBOSE, "Stage out");
    FileTransfer ft(configurator.getCachePath());
    ft.upload(configurator.getJobRoot(), j);
    return true;
}

}
