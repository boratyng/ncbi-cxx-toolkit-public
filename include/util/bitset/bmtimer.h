#ifndef BMTIMER__H__INCLUDED__
#define BMTIMER__H__INCLUDED__
/*
Copyright(c) 2002-2017 Anatoliy Kuznetsov(anatoliy_kuznetsov at yahoo.com)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

For more information please visit:  http://bitmagic.io
*/

/*! \file bmtimer.h
    \brief Timing utilities for benchmarking (internal)
*/

#include <iostream>
#include <iomanip>
#include <string>
#include <map>
#include <chrono>

namespace bm
{


/// Utility class to collect performance measurements and statistics.
///
/// @internal
///
class chrono_taker
{
public:
    /// collected statistics
    ///
    struct statistics
    {
        std::chrono::duration<double, std::milli>  duration;
        unsigned                                   repeats;
        
        statistics() : repeats(1) {}
        
        statistics(std::chrono::duration<double, std::milli> d, unsigned r)
        : duration(d), repeats(r)
        {}
    };
    
    enum format
    {
        ct_time = 0,
        ct_ops_per_sec,
        ct_all
    };
    
    /// test name to duration map
    ///
    typedef std::map<std::string, statistics > duration_map_type;

public:
    chrono_taker(const std::string name,
                unsigned repeats = 1,
                duration_map_type* dmap = 0)
    : name_(name),
      repeats_(repeats),
      dmap_(dmap),
      is_stopped_(false)
    {
        start_ = std::chrono::steady_clock::now();
    }
    
    ~chrono_taker()
    {
        try
        {
            if (!is_stopped_)
            {
                stop();
            }
        }
        catch(...)
        {}
    }
    
    
    void stop(bool silent=false)
    {
        finish_ = std::chrono::steady_clock::now();
        auto diff = finish_ - start_;
        if (dmap_)
        {
            statistics st(diff, repeats_);
            duration_map_type::iterator it = dmap_->find(name_);
            if (it == dmap_->end())
            {
                (*dmap_)[name_] = st;
            }
            else
            {
                it->second.repeats++;
                it->second.duration += st.duration;
            }
        }
        else // report the measurements
        {
            if (!silent)
                std::cout << name_ << ": "
                          << std::chrono::duration <double, std::milli> (diff).count()
                          << std::endl;
        }
        is_stopped_ = true;
    }
    
    void add_repeats(unsigned inc)
    {
        repeats_ += inc;
    }

    
    static void print_duration_map(const duration_map_type& dmap, format fmt = ct_time)
    {
        duration_map_type::const_iterator it = dmap.begin();
        duration_map_type::const_iterator it_end = dmap.end();
        
        for ( ;it != it_end; ++it)
        {
            const chrono_taker::statistics& st = it->second;
            format f;
            if (st.repeats <= 1)
                f = ct_time;
            else
                f = fmt;

            switch (f)
            {
            case ct_time:
            print_time:
                {
                auto ms = it->second.duration.count();
                if (ms > 1000)
                {
                    double sec = ms / 1000;
                    std::cout << it->first << "; " << std::setprecision(4) << sec << " sec" << std::endl;
                }
                else
                    std::cout << it->first << "; " << it->second.duration.count() << " ms" << std::endl;
                }
                break;
            case ct_ops_per_sec:
                {
                unsigned iops = (unsigned)((double)st.repeats / (double)it->second.duration.count()) * 1000;
                if (iops)
                {
                    std::cout << it->first << "; " << iops << " ops/sec" << std::endl;
                }
                else
                {
                    double ops = ((double)st.repeats / (double)it->second.duration.count()) * 1000;
                    std::cout << it->first << "; " << std::setprecision(4) << ops << " ops/sec" << std::endl;
                }
                }
                break;
            case ct_all:
                {
                if (st.repeats <= 1)
                {
                    goto print_time;
                }
                unsigned iops = (unsigned)((double)st.repeats / (double)it->second.duration.count()) * 1000;
                if (iops)
                {
                    std::cout << it->first << "; " << iops << " ops/sec; "
                              << std::setprecision(4) << it->second.duration.count() << " ms" << std::endl;
                }
                else
                {
                    double sec = double(it->second.duration.count()) / 1000;

                    double ops = ((double)st.repeats / (double)it->second.duration.count()) * 1000;
                    std::cout << it->first << "; " << std::setprecision(4) << ops << " ops/sec; "
                              << std::setprecision(4) << sec << " sec." << std::endl;
                }
                }
                break;

            default:
                break;
            }
        } // for
    }
    

    chrono_taker(const chrono_taker&) = delete;
    chrono_taker & operator=(const chrono_taker) = delete;
    
protected:
    std::string                                        name_;
    std::chrono::time_point<std::chrono::steady_clock> start_;
    std::chrono::time_point<std::chrono::steady_clock> finish_;
    unsigned                                           repeats_;
    duration_map_type*                                 dmap_;
    bool                                               is_stopped_;
};


} // namespace

#endif
