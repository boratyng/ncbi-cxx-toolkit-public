/*  $Id$
 * ===========================================================================
 *
 *                            PUBLIC DOMAIN NOTICE
 *               National Center for Biotechnology Information
 *
 *  This software/database is a "United States Government Work" under the
 *  terms of the United States Copyright Act.  It was written as part of
 *  the author's official duties as a United States Government employee and
 *  thus cannot be copyrighted.  This software/database is freely available
 *  to the public for use. The National Library of Medicine and the U.S.
 *  Government have not placed any restriction on its use or reproduction.
 *
 *  Although all reasonable efforts have been taken to ensure the accuracy
 *  and reliability of the software and data, the NLM and the U.S.
 *  Government do not and cannot warrant the performance or results that
 *  may be obtained by using this software or data. The NLM and the U.S.
 *  Government disclaim all warranties, express or implied, including
 *  warranties of performance, merchantability or fitness for any particular
 *  purpose.
 *
 *  Please cite the author in any work or product based on this material.
 *
 * ===========================================================================
 *
 * Authors: Sergey Satskiy
 *
 * File Description:
 *
 */
#include <ncbi_pch.hpp>

#include <corelib/ncbi_config.hpp>
#include <corelib/plugin_manager.hpp>

#include "settings.hpp"
#include "pubseq_gateway_exception.hpp"
#include "pubseq_gateway_logging.hpp"
#include "alerts.hpp"
#include "timing.hpp"


const string            kServerSection = "SERVER";
const string            kLmdbCacheSection = "LMDB_CACHE";
const string            kStatisticsSection = "STATISTICS";
const string            kAutoExcludeSection = "AUTO_EXCLUDE";
const string            kDebugSection = "DEBUG";
const string            kIPGSection = "IPG";
const string            kSSLSection = "SSL";
const string            kHealthSection = "HEALTH";
const string            kOSGProcessorSection = "OSG_PROCESSOR";
const string            kCDDProcessorSection = "CDD_PROCESSOR";
const string            kWGSProcessorSection = "WGS_PROCESSOR";
const string            kSNPProcessorSection = "SNP_PROCESSOR";
const string            kCassandraProcessorSection = "CASSANDRA_PROCESSOR";
const string            kAdminSection = "ADMIN";
const string            kMyNCBISection = "MY_NCBI";
const string            kCountersSection = "COUNTERS";


const unsigned short    kWorkersDefault = 64;
const unsigned int      kListenerBacklogDefault = 256;
const unsigned short    kTcpMaxConnDefault = 4096;
const unsigned int      kTimeoutDefault = 30000;
const unsigned int      kMaxRetriesDefault = 2;
const string            kDefaultRootKeyspace = "sat_info3";
const string            kDefaultConfigurationDomain = "PSG";
const size_t            kDefaultHttpMaxBacklog = 1024;
const size_t            kDefaultHttpMaxRunning = 64;
const size_t            kDefaultLogSamplingRatio = 0;
const size_t            kDefaultLogTimingThreshold = 1000;
const unsigned long     kDefaultSendBlobIfSmall = 10 * 1024;
const unsigned long     kDefaultSmallBlobSize = 16;
const bool              kDefaultLog = true;
const unsigned int      kDefaultExcludeCacheMaxSize = 1000;
const unsigned int      kDefaultExcludeCachePurgePercentage = 20;
const unsigned int      kDefaultExcludeCacheInactivityPurge = 60;
const unsigned int      kDefaultMaxHops = 2;
const bool              kDefaultAllowIOTest = false;
const bool              kDefaultAllowProcessorTiming = false;
const string            kDefaultOnlyForProcessor = "";
const double            kDefaultResendTimeoutSec = 0.2;
const double            kDefaultRequestTimeoutSec = 30.0;
const size_t            kDefaultProcessorMaxConcurrency = 1200;
const size_t            kDefaultSplitInfoBlobCacheSize = 1000;
const size_t            kDefaultUserInfoCacheSize = 100;
const size_t            kDefaultIPGPageSize = 1024;
const bool              kDefaultEnableHugeIPG = true;
const string            kDefaultAuthToken = "";
const bool              kDefaultSSLEnable = false;
const string            kDefaultSSLCertFile = "";
const string            kDefaultSSLKeyFile = "";
const string            kDefaultSSLCiphers = "EECDH+aRSA+AESGCM EDH+aRSA+AESGCM EECDH+aRSA EDH+aRSA !SHA !SHA256 !SHA384";
const size_t            kDefaultShutdownIfTooManyOpenFDforHTTP = 0;
const size_t            kDefaultShutdownIfTooManyOpenFDforHTTPS = 8000;
const string            kDefaultTestSeqId = "gi|2";
const bool              kDefaultTestSeqIdIgnoreError = true;
const bool              kDefaultCassandraProcessorsEnabled = true;
const bool              kDefaultOSGProcessorsEnabled = false;
const bool              kDefaultCDDProcessorsEnabled = true;
const bool              kDefaultWGSProcessorsEnabled = true;
const bool              kDefaultSNPProcessorsEnabled = true;
const string            kDefaultMyNCBIURL = "http://txproxy.linkerd.ncbi.nlm.nih.gov/v1/service/MyNCBIAccount?txsvc=MyNCBIAccount";
const string            kDefaultMyNCBIHttpProxy = "linkerd:4140";
size_t                  kDefaultMyNCBITimeoutMs = 100;



SPubseqGatewaySettings::SPubseqGatewaySettings() :
    m_HttpPort(0),
    m_HttpWorkers(kWorkersDefault),
    m_ListenerBacklog(kListenerBacklogDefault),
    m_TcpMaxConn(kTcpMaxConnDefault),
    m_TimeoutMs(kTimeoutDefault),
    m_MaxRetries(kMaxRetriesDefault),
    m_SendBlobIfSmall(kDefaultSendBlobIfSmall),
    m_Log(kDefaultLog),
    m_MaxHops(kDefaultMaxHops),
    m_ResendTimeoutSec(kDefaultResendTimeoutSec),
    m_RequestTimeoutSec(kDefaultRequestTimeoutSec),
    m_ProcessorMaxConcurrency(kDefaultProcessorMaxConcurrency),
    m_SplitInfoBlobCacheSize(kDefaultSplitInfoBlobCacheSize),
    m_UserInfoCacheSize(kDefaultUserInfoCacheSize),
    m_ShutdownIfTooManyOpenFD(0),
    m_RootKeyspace(kDefaultRootKeyspace),
    m_ConfigurationDomain(kDefaultConfigurationDomain),
    m_HttpMaxBacklog(kDefaultHttpMaxBacklog),
    m_HttpMaxRunning(kDefaultHttpMaxRunning),
    m_LogSamplingRatio(kDefaultLogSamplingRatio),
    m_LogTimingThreshold(kDefaultLogTimingThreshold),
    m_SmallBlobSize(kDefaultSmallBlobSize),
    m_MinStatValue(kMinStatValue),
    m_MaxStatValue(kMaxStatValue),
    m_NStatBins(kNStatBins),
    m_StatScaleType(kStatScaleType),
    m_TickSpan(kTickSpan),
    m_OnlyForProcessor(kDefaultOnlyForProcessor),
    m_ExcludeCacheMaxSize(kDefaultExcludeCacheMaxSize),
    m_ExcludeCachePurgePercentage(kDefaultExcludeCachePurgePercentage),
    m_ExcludeCacheInactivityPurge(kDefaultExcludeCacheInactivityPurge),
    m_AllowIOTest(kDefaultAllowIOTest),
    m_AllowProcessorTiming(kDefaultAllowProcessorTiming),
    m_SSLEnable(kDefaultSSLEnable),
    m_SSLCiphers(kDefaultSSLCiphers),
    m_TestSeqId(kDefaultTestSeqId),
    m_TestSeqIdIgnoreError(kDefaultTestSeqIdIgnoreError),
    m_CassandraProcessorsEnabled(kDefaultCassandraProcessorsEnabled),
    m_OSGProcessorsEnabled(kDefaultOSGProcessorsEnabled),
    m_CDDProcessorsEnabled(kDefaultCDDProcessorsEnabled),
    m_WGSProcessorsEnabled(kDefaultWGSProcessorsEnabled),
    m_SNPProcessorsEnabled(kDefaultSNPProcessorsEnabled)
{}


SPubseqGatewaySettings::~SPubseqGatewaySettings()
{}


void SPubseqGatewaySettings::Read(const CNcbiRegistry &   registry,
                                  CPSGAlerts &  alerts)
{
    // Note: reading of some values in the [SERVER] depends if SSL is on/off
    //       So, reading of the SSL settings is done first
    x_ReadSSLSection(registry);

    x_ReadServerSection(registry);
    x_ReadStatisticsSection(registry);
    x_ReadLmdbCacheSection(registry);
    x_ReadAutoExcludeSection(registry);
    x_ReadIPGSection(registry);
    x_ReadDebugSection(registry);
    x_ReadHealthSection(registry);
    x_ReadAdminSection(registry, alerts);
    x_ReadCassandraProcessorSection(registry);
    x_ReadOSGProcessorSection(registry);
    x_ReadCDDProcessorSection(registry);
    x_ReadWGSProcessorSection(registry);
    x_ReadSNPProcessorSection(registry);
    x_ReadMyNCBISection(registry);
    x_ReadCountersSection(registry);
}


void SPubseqGatewaySettings::x_ReadServerSection(const CNcbiRegistry &   registry)
{

    if (!registry.HasEntry(kServerSection, "port"))
        NCBI_THROW(CPubseqGatewayException, eConfigurationError,
                   "[" + kServerSection +
                   "]/port value is not found in the configuration "
                   "file. The port must be provided to run the server. "
                   "Exiting.");

    m_HttpPort = registry.GetInt(kServerSection, "port", 0);
    m_HttpWorkers = registry.GetInt(kServerSection, "workers",
                                    kWorkersDefault);
    m_ListenerBacklog = registry.GetInt(kServerSection, "backlog",
                                        kListenerBacklogDefault);
    m_TcpMaxConn = registry.GetInt(kServerSection, "maxconn",
                                   kTcpMaxConnDefault);
    m_TimeoutMs = registry.GetInt(kServerSection, "optimeout",
                                  kTimeoutDefault);
    m_MaxRetries = registry.GetInt(kServerSection, "maxretries",
                                   kMaxRetriesDefault);
    m_RootKeyspace = registry.GetString(kServerSection, "root_keyspace",
                                        kDefaultRootKeyspace);
    m_ConfigurationDomain = registry.GetString(kServerSection, "configuration_domain",
                                               kDefaultConfigurationDomain);
    m_HttpMaxBacklog = registry.GetInt(kServerSection, "http_max_backlog",
                                       kDefaultHttpMaxBacklog);
    m_HttpMaxRunning = registry.GetInt(kServerSection, "http_max_running",
                                       kDefaultHttpMaxRunning);
    m_LogSamplingRatio = registry.GetInt(kServerSection, "log_sampling_ratio",
                                         kDefaultLogSamplingRatio);
    m_LogTimingThreshold = registry.GetInt(kServerSection, "log_timing_threshold",
                                           kDefaultLogTimingThreshold);
    m_SendBlobIfSmall = x_GetDataSize(registry, kServerSection,
                                      "send_blob_if_small",
                                      kDefaultSendBlobIfSmall);
    m_Log = registry.GetBool(kServerSection, "log", kDefaultLog);
    m_MaxHops = registry.GetInt(kServerSection, "max_hops", kDefaultMaxHops);
    m_ResendTimeoutSec = registry.GetDouble(kServerSection, "resend_timeout",
                                            kDefaultResendTimeoutSec);
    m_RequestTimeoutSec = registry.GetDouble(kServerSection, "request_timeout",
                                             kDefaultRequestTimeoutSec);
    m_ProcessorMaxConcurrency = registry.GetInt(kServerSection,
                                                "ProcessorMaxConcurrency",
                                                kDefaultProcessorMaxConcurrency);
    m_SplitInfoBlobCacheSize = registry.GetInt(kServerSection,
                                               "split_info_blob_cache_size",
                                               kDefaultSplitInfoBlobCacheSize);
    m_UserInfoCacheSize = registry.GetInt(kServerSection,
                                          "user_info_cache_size",
                                           kDefaultUserInfoCacheSize);

    if (m_SSLEnable) {
        m_ShutdownIfTooManyOpenFD =
                registry.GetInt(kServerSection, "ShutdownIfTooManyOpenFD",
                                kDefaultShutdownIfTooManyOpenFDforHTTPS);
    } else {
        m_ShutdownIfTooManyOpenFD =
                registry.GetInt(kServerSection, "ShutdownIfTooManyOpenFD",
                                kDefaultShutdownIfTooManyOpenFDforHTTP);
    }
}


void SPubseqGatewaySettings::x_ReadStatisticsSection(const CNcbiRegistry &   registry)
{
    m_SmallBlobSize = x_GetDataSize(registry, kStatisticsSection,
                                    "small_blob_size", kDefaultSmallBlobSize);
    m_MinStatValue = registry.GetInt(kStatisticsSection,
                                     "min", kMinStatValue);
    m_MaxStatValue = registry.GetInt(kStatisticsSection,
                                     "max", kMaxStatValue);
    m_NStatBins = registry.GetInt(kStatisticsSection,
                                  "n_bins", kNStatBins);
    m_StatScaleType = registry.GetString(kStatisticsSection,
                                         "type", kStatScaleType);
    m_TickSpan = registry.GetInt(kStatisticsSection,
                                 "tick_span", kTickSpan);
    m_OnlyForProcessor = registry.GetString(kStatisticsSection,
                                            "only_for_processor",
                                            kDefaultOnlyForProcessor);
}


void SPubseqGatewaySettings::x_ReadLmdbCacheSection(const CNcbiRegistry &   registry)
{
    m_Si2csiDbFile = registry.GetString(kLmdbCacheSection,
                                        "dbfile_si2csi", "");
    m_BioseqInfoDbFile = registry.GetString(kLmdbCacheSection,
                                            "dbfile_bioseq_info", "");
    m_BlobPropDbFile = registry.GetString(kLmdbCacheSection,
                                          "dbfile_blob_prop", "");
}


void SPubseqGatewaySettings::x_ReadAutoExcludeSection(const CNcbiRegistry &   registry)
{
    m_ExcludeCacheMaxSize = registry.GetInt(
                            kAutoExcludeSection, "max_cache_size",
                            kDefaultExcludeCacheMaxSize);
    m_ExcludeCachePurgePercentage = registry.GetInt(
                            kAutoExcludeSection, "purge_percentage",
                            kDefaultExcludeCachePurgePercentage);
    m_ExcludeCacheInactivityPurge = registry.GetInt(
                            kAutoExcludeSection, "inactivity_purge_timeout",
                            kDefaultExcludeCacheInactivityPurge);
}


void SPubseqGatewaySettings::x_ReadDebugSection(const CNcbiRegistry &   registry)
{
    m_AllowIOTest = registry.GetBool(kDebugSection, "psg_allow_io_test",
                                     kDefaultAllowIOTest);
    m_AllowProcessorTiming = registry.GetBool(kDebugSection, "allow_processor_timing",
                                              kDefaultAllowProcessorTiming);
}


void SPubseqGatewaySettings::x_ReadIPGSection(const CNcbiRegistry &   registry)
{
    m_IPGPageSize = registry.GetInt(kIPGSection, "page_size",
                                    kDefaultIPGPageSize);
    m_EnableHugeIPG = registry.GetBool(kIPGSection, "enable_huge_ipg",
                                       kDefaultEnableHugeIPG);
}


void SPubseqGatewaySettings::x_ReadSSLSection(const CNcbiRegistry &   registry)
{
    m_SSLEnable = registry.GetBool(kSSLSection,
                                   "ssl_enable", kDefaultSSLEnable);
    m_SSLCertFile = registry.GetString(kSSLSection,
                                       "ssl_cert_file", kDefaultSSLCertFile);
    m_SSLKeyFile = registry.GetString(kSSLSection,
                                      "ssl_key_file", kDefaultSSLKeyFile);
    m_SSLCiphers = registry.GetString(kSSLSection,
                                      "ssl_ciphers", kDefaultSSLCiphers);
}


void SPubseqGatewaySettings::x_ReadHealthSection(const CNcbiRegistry &   registry)
{
    m_TestSeqId = registry.GetString(kHealthSection,
                                     "test_seq_id", kDefaultTestSeqId);
    m_TestSeqIdIgnoreError = registry.GetBool(kHealthSection,
                                              "test_seq_id_ignore_error",
                                              kDefaultTestSeqIdIgnoreError);
}


void SPubseqGatewaySettings::x_ReadCassandraProcessorSection(const CNcbiRegistry &   registry)
{
    m_CassandraProcessorsEnabled =
            registry.GetBool(kCassandraProcessorSection, "enabled",
                             kDefaultCassandraProcessorsEnabled);
}


void SPubseqGatewaySettings::x_ReadOSGProcessorSection(const CNcbiRegistry &   registry)
{
    m_OSGProcessorsEnabled = registry.GetBool(kOSGProcessorSection,
                                              "enabled",
                                              kDefaultOSGProcessorsEnabled);
}


void SPubseqGatewaySettings::x_ReadCDDProcessorSection(const CNcbiRegistry &   registry)
{
    m_CDDProcessorsEnabled = registry.GetBool(kCDDProcessorSection,
                                              "enabled",
                                              kDefaultCDDProcessorsEnabled);
}


void SPubseqGatewaySettings::x_ReadWGSProcessorSection(const CNcbiRegistry &   registry)
{
    m_WGSProcessorsEnabled = registry.GetBool(kWGSProcessorSection,
                                              "enabled",
                                              kDefaultWGSProcessorsEnabled);
}


void SPubseqGatewaySettings::x_ReadSNPProcessorSection(const CNcbiRegistry &   registry)
{
    m_SNPProcessorsEnabled = registry.GetBool(kSNPProcessorSection,
                                              "enabled",
                                              kDefaultSNPProcessorsEnabled);
}


void SPubseqGatewaySettings::x_ReadMyNCBISection(const CNcbiRegistry &   registry)
{
    m_MyNCBIURL = registry.GetString(kMyNCBISection,
                                     "url",
                                     kDefaultMyNCBIURL);
    m_MyNCBIHttpProxy = registry.GetString(kMyNCBISection,
                                           "http_proxy",
                                           kDefaultMyNCBIHttpProxy);
    m_MyNCBITimeoutMs = registry.GetInt(kMyNCBISection,
                                        "timeout_ms",
                                        kDefaultMyNCBITimeoutMs);
}


void SPubseqGatewaySettings::x_ReadCountersSection(const CNcbiRegistry &   registry)
{
    list<string>            entries;
    registry.EnumerateEntries(kCountersSection, &entries);

    for(const auto &  value_id : entries) {
        string      name_and_description = registry.Get(kCountersSection,
                                                        value_id);
        string      name;
        string      description;
        if (NStr::SplitInTwo(name_and_description, ":::", name, description,
                             NStr::fSplit_ByPattern)) {
            m_IdToNameAndDescription[value_id] = {name, description};
        } else {
            PSG_WARNING("Malformed counter [" << kCountersSection << "]/" <<
                        name << " information. Expected <name>:::<description");
        }
    }
}


void SPubseqGatewaySettings::x_ReadAdminSection(const CNcbiRegistry &   registry,
                                                CPSGAlerts &  alerts)
{
    try {
        m_AuthToken = registry.GetEncryptedString(kAdminSection, "auth_token",
                                                  IRegistry::fPlaintextAllowed);
    } catch (const CRegistryException &  ex) {
        string  msg = "Decrypting error detected while reading "
                      "[" + kAdminSection + "]/auth_token value: " +
                      string(ex.what());
        ERR_POST(msg);
        alerts.Register(ePSGS_ConfigAuthDecrypt, msg);

        // Treat the value as a clear text
        m_AuthToken = registry.GetString("ADMIN", "auth_token",
                                         kDefaultAuthToken);
    } catch (...) {
        string  msg = "Unknown decrypting error detected while reading "
                      "[" + kAdminSection + "]/auth_token value";
        ERR_POST(msg);
        alerts.Register(ePSGS_ConfigAuthDecrypt, msg);

        // Treat the value as a clear text
        m_AuthToken = registry.GetString("ADMIN", "auth_token",
                                         kDefaultAuthToken);
    }
}


void SPubseqGatewaySettings::Validate(CPSGAlerts &  alerts)
{
    const unsigned short    kHttpPortMin = 1;
    const unsigned short    kHttpPortMax = 65534;
    const unsigned short    kWorkersMin = 1;
    const unsigned short    kWorkersMax = 100;
    const unsigned int      kListenerBacklogMin = 5;
    const unsigned int      kListenerBacklogMax = 2048;
    const unsigned short    kTcpMaxConnMax = 65000;
    const unsigned short    kTcpMaxConnMin = 5;
    const unsigned int      kTimeoutMsMin = 0;
    const unsigned int      kTimeoutMsMax = UINT_MAX;
    const unsigned int      kMaxRetriesMin = 0;
    const unsigned int      kMaxRetriesMax = UINT_MAX;


    if (m_HttpPort < kHttpPortMin || m_HttpPort > kHttpPortMax) {
        NCBI_THROW(CPubseqGatewayException, eConfigurationError,
                   "[" + kServerSection +
                   "]/port value is out of range. Allowed range: " +
                   to_string(kHttpPortMin) + "..." +
                   to_string(kHttpPortMax) + ". Received: " +
                   to_string(m_HttpPort));
    }

    if (m_Si2csiDbFile.empty()) {
        PSG_WARNING("[" + kLmdbCacheSection + "]/dbfile_si2csi is not found "
                    "in the ini file. No si2csi cache will be used.");
    }

    if (m_BioseqInfoDbFile.empty()) {
        PSG_WARNING("[" + kLmdbCacheSection + "]/dbfile_bioseq_info is not found "
                    "in the ini file. No bioseq_info cache will be used.");
    }

    if (m_BlobPropDbFile.empty()) {
        PSG_WARNING("[" + kLmdbCacheSection + "]/dbfile_blob_prop is not found "
                    "in the ini file. No blob_prop cache will be used.");
    }

    if (m_HttpWorkers < kWorkersMin || m_HttpWorkers > kWorkersMax) {
        string  err_msg =
            "The number of HTTP workers is out of range. Allowed "
            "range: " + to_string(kWorkersMin) + "..." +
            to_string(kWorkersMax) + ". Received: " +
            to_string(m_HttpWorkers) + ". Reset to "
            "default: " + to_string(kWorkersDefault);
        alerts.Register(ePSGS_ConfigHttpWorkers, err_msg);
        PSG_ERROR(err_msg);
        m_HttpWorkers = kWorkersDefault;
    }

    if (m_ListenerBacklog < kListenerBacklogMin ||
        m_ListenerBacklog > kListenerBacklogMax) {
        string  err_msg =
            "The listener backlog is out of range. Allowed "
            "range: " + to_string(kListenerBacklogMin) + "..." +
            to_string(kListenerBacklogMax) + ". Received: " +
            to_string(m_ListenerBacklog) + ". Reset to "
            "default: " + to_string(kListenerBacklogDefault);
        alerts.Register(ePSGS_ConfigListenerBacklog, err_msg);
        PSG_ERROR(err_msg);
        m_ListenerBacklog = kListenerBacklogDefault;
    }

    if (m_TcpMaxConn < kTcpMaxConnMin || m_TcpMaxConn > kTcpMaxConnMax) {
        string  err_msg =
            "The max number of connections is out of range. Allowed "
            "range: " + to_string(kTcpMaxConnMin) + "..." +
            to_string(kTcpMaxConnMax) + ". Received: " +
            to_string(m_TcpMaxConn) + ". Reset to "
            "default: " + to_string(kTcpMaxConnDefault);
        alerts.Register(ePSGS_ConfigMaxConnections, err_msg);
        PSG_ERROR(err_msg);
        m_TcpMaxConn = kTcpMaxConnDefault;
    }

    if (m_TimeoutMs < kTimeoutMsMin || m_TimeoutMs > kTimeoutMsMax) {
        string  err_msg =
            "The operation timeout is out of range. Allowed "
            "range: " + to_string(kTimeoutMsMin) + "..." +
            to_string(kTimeoutMsMax) + ". Received: " +
            to_string(m_TimeoutMs) + ". Reset to "
            "default: " + to_string(kTimeoutDefault);
        alerts.Register(ePSGS_ConfigTimeout, err_msg);
        PSG_ERROR(err_msg);
        m_TimeoutMs = kTimeoutDefault;
    }

    if (m_MaxRetries < kMaxRetriesMin || m_MaxRetries > kMaxRetriesMax) {
        string  err_msg =
            "The max retries is out of range. Allowed "
            "range: " + to_string(kMaxRetriesMin) + "..." +
            to_string(kMaxRetriesMax) + ". Received: " +
            to_string(m_MaxRetries) + ". Reset to "
            "default: " + to_string(kMaxRetriesDefault);
        alerts.Register(ePSGS_ConfigRetries, err_msg);
        PSG_ERROR(err_msg);
        m_MaxRetries = kMaxRetriesDefault;
    }

    if (m_ExcludeCacheMaxSize < 0) {
        string  err_msg =
            "The max exclude cache size must be a positive integer. "
            "Received: " + to_string(m_ExcludeCacheMaxSize) + ". "
            "Reset to 0 (exclude blobs cache is disabled)";
        alerts.Register(ePSGS_ConfigExcludeCacheSize, err_msg);
        PSG_ERROR(err_msg);
        m_ExcludeCacheMaxSize = 0;
    }

    if (m_ExcludeCachePurgePercentage < 0 || m_ExcludeCachePurgePercentage > 100) {
        string  err_msg = "The exclude cache purge percentage is out of range. "
            "Allowed: 0...100. Received: " +
            to_string(m_ExcludeCachePurgePercentage) + ". ";
        if (m_ExcludeCacheMaxSize > 0) {
            err_msg += "Reset to " +
                to_string(kDefaultExcludeCachePurgePercentage);
            PSG_ERROR(err_msg);
        } else {
            err_msg += "The provided value has no effect "
                "because the exclude cache is disabled.";
            PSG_WARNING(err_msg);
        }
        m_ExcludeCachePurgePercentage = kDefaultExcludeCachePurgePercentage;
        alerts.Register(ePSGS_ConfigExcludeCachePurgeSize, err_msg);
    }

    if (m_ExcludeCacheInactivityPurge <= 0) {
        string  err_msg = "The exclude cache inactivity purge timeout must be "
            "a positive integer greater than zero. Received: " +
            to_string(m_ExcludeCacheInactivityPurge) + ". ";
        if (m_ExcludeCacheMaxSize > 0) {
            err_msg += "Reset to " +
                to_string(kDefaultExcludeCacheInactivityPurge);
            PSG_ERROR(err_msg);
        } else {
            err_msg += "The provided value has no effect "
                "because the exclude cache is disabled.";
            PSG_WARNING(err_msg);
        }
        m_ExcludeCacheInactivityPurge = kDefaultExcludeCacheInactivityPurge;
        alerts.Register(ePSGS_ConfigExcludeCacheInactivity, err_msg);
    }

    if (m_HttpMaxBacklog <= 0) {
        PSG_WARNING("Invalid " + kServerSection + "]/http_max_backlog value (" +
                    to_string(m_HttpMaxBacklog) + "). "
                    "The http max backlog must be greater than 0. The http max backlog is "
                    "reset to the default value (" +
                    to_string(kDefaultHttpMaxBacklog) + ").");
        m_HttpMaxBacklog = kDefaultHttpMaxBacklog;
    }

    if (m_HttpMaxRunning <= 0) {
        PSG_WARNING("Invalid " + kServerSection + "]/http_max_running value (" +
                    to_string(m_HttpMaxRunning) + "). "
                    "The http max running must be greater than 0. The http max running is "
                    "reset to the default value (" +
                    to_string(kDefaultHttpMaxRunning) + ").");
        m_HttpMaxRunning = kDefaultHttpMaxRunning;
    }

    if (m_LogSamplingRatio < 0) {
        PSG_WARNING("Invalid " + kServerSection + "]/log_sampling_ratio value (" +
                    to_string(m_LogSamplingRatio) + "). "
                    "The log sampling ratio must be greater or equal 0. The log sampling ratio is "
                    "reset to the default value (" +
                    to_string(kDefaultLogSamplingRatio) + ").");
        m_LogSamplingRatio = kDefaultLogSamplingRatio;
    }

    if (m_LogTimingThreshold < 0) {
        PSG_WARNING("Invalid " + kServerSection + "]/log_timing_threshold value (" +
                    to_string(m_LogTimingThreshold) + "). "
                    "The log timing threshold must be greater or equal 0. The log timing threshold is "
                    "reset to the default value (" +
                    to_string(kDefaultLogTimingThreshold) + ").");
        m_LogTimingThreshold = kDefaultLogTimingThreshold;
    }

    if (m_MaxHops <= 0) {
        PSG_WARNING("Invalid " + kServerSection + "]/max_hops value (" +
                    to_string(m_MaxHops) + "). "
                    "The max hops must be greater than 0. The max hops is "
                    "reset to the default value (" +
                    to_string(kDefaultMaxHops) + ").");
        m_MaxHops = kDefaultMaxHops;
    }

    bool        stat_settings_good = true;
    if (NStr::CompareNocase(m_StatScaleType, "log") != 0 &&
        NStr::CompareNocase(m_StatScaleType, "linear") != 0) {
        string  err_msg = "Invalid [" + kStatisticsSection +
            "]/type value '" + m_StatScaleType +
            "'. Allowed values are: log, linear. "
            "The statistics parameters are reset to default.";
        alerts.Register(ePSGS_ConfigStatScaleType, err_msg);
        PSG_ERROR(err_msg);
        stat_settings_good = false;

        m_MinStatValue = kMinStatValue;
        m_MaxStatValue = kMaxStatValue;
        m_NStatBins = kNStatBins;
        m_StatScaleType = kStatScaleType;
    }

    if (stat_settings_good) {
        if (m_MinStatValue > m_MaxStatValue) {
            string  err_msg = "Invalid [" + kStatisticsSection +
                "]/min and max values. The "
                "max cannot be less than min. "
                "The statistics parameters are reset to default.";
            alerts.Register(ePSGS_ConfigStatMinMaxVal, err_msg);
            PSG_ERROR(err_msg);
            stat_settings_good = false;

            m_MinStatValue = kMinStatValue;
            m_MaxStatValue = kMaxStatValue;
            m_NStatBins = kNStatBins;
            m_StatScaleType = kStatScaleType;
        }
    }

    if (stat_settings_good) {
        if (m_NStatBins <= 0) {
            string  err_msg = "Invalid [" + kStatisticsSection +
                "]/n_bins value. The "
                "number of bins must be greater than 0. "
                "The statistics parameters are reset to default.";
            alerts.Register(ePSGS_ConfigStatNBins, err_msg);
            PSG_ERROR(err_msg);

            // Uncomment if there will be more [STATISTICS] section parameters
            // stat_settings_good = false;

            m_MinStatValue = kMinStatValue;
            m_MaxStatValue = kMaxStatValue;
            m_NStatBins = kNStatBins;
            m_StatScaleType = kStatScaleType;
        }
    }

    if (m_TickSpan <= 0) {
        PSG_WARNING("Invalid [" + kStatisticsSection + "]/tick_span value (" +
                    to_string(m_TickSpan) + "). "
                    "The tick span must be greater than 0. The tick span is "
                    "reset to the default value (" +
                    to_string(kTickSpan) + ").");
        m_TickSpan = kTickSpan;
    }

    if (m_ResendTimeoutSec < 0.0) {
        PSG_WARNING("Invalid [" + kServerSection + "]/resend_timeout value (" +
                    to_string(m_ResendTimeoutSec) + "). "
                    "The resend timeout must be greater or equal to 0. The resend "
                    "timeout is reset to the default value (" +
                    to_string(kDefaultResendTimeoutSec) + ").");
        m_ResendTimeoutSec = kDefaultResendTimeoutSec;
    }

    if (m_RequestTimeoutSec <= 0.0) {
        PSG_WARNING("Invalid [" + kServerSection + "]/request_timeout value (" +
                    to_string(m_RequestTimeoutSec) + "). "
                    "The request timeout must be greater than 0. The request "
                    "timeout is reset to the default value (" +
                    to_string(kDefaultRequestTimeoutSec) + ").");
        m_RequestTimeoutSec = kDefaultRequestTimeoutSec;
    }

    if (m_ProcessorMaxConcurrency == 0) {
        PSG_WARNING("Invalid [" + kServerSection +
                    "]/ProcessorMaxConcurrency value (" +
                    to_string(m_ProcessorMaxConcurrency) + "). "
                    "The processor max concurrency must be greater than 0. "
                    "The processor max concurrency is reset to the default value (" +
                    to_string(kDefaultProcessorMaxConcurrency) + ").");
        m_RequestTimeoutSec = kDefaultProcessorMaxConcurrency;
    }

    if (m_IPGPageSize <= 0) {
        PSG_WARNING("The [" + kIPGSection + "]/page_size value must be > 0. "
                    "The [" + kIPGSection + "]/page_size is switched to the "
                    "default value: " + to_string(kDefaultIPGPageSize));
        m_IPGPageSize = kDefaultIPGPageSize;
    }

    if (m_SSLEnable) {
        if (m_SSLCertFile.empty()) {
            NCBI_THROW(CPubseqGatewayException, eConfigurationError,
                       "[" + kSSLSection + "]/ssl_cert_file value must be provided "
                       "if [" + kSSLSection + "]/ssl_enable is set to true");
        }
        if (m_SSLKeyFile.empty()) {
            NCBI_THROW(CPubseqGatewayException, eConfigurationError,
                       "[" + kSSLSection + "]/ssl_key_file value must be provided "
                       "if [" + kSSLSection + "]/ssl_enable is set to true");
        }

        if (!CFile(m_SSLCertFile).Exists()) {
            NCBI_THROW(CPubseqGatewayException, eConfigurationError,
                       "[" + kSSLSection + "]/ssl_cert_file is not found");
        }
        if (!CFile(m_SSLKeyFile).Exists()) {
            NCBI_THROW(CPubseqGatewayException, eConfigurationError,
                       "[" + kSSLSection + "]/ssl_key_file is not found");
        }

        if (m_SSLCiphers.empty()) {
            m_SSLCiphers = kDefaultSSLCiphers;
        }
    }

    if (m_MyNCBIURL.empty()) {
        PSG_WARNING("The [" + kMyNCBISection + "]/url value must be not empty. "
                    "The [" + kMyNCBISection + "]/url is switched to the "
                    "default value: " + kDefaultMyNCBIURL);
        m_MyNCBIURL = kDefaultMyNCBIURL;
    }

    if (m_MyNCBITimeoutMs <= 0) {
        PSG_WARNING("The [" + kMyNCBISection + "]/timeout_ms value must be > 0. "
                    "The [" + kMyNCBISection + "]/timeout_ms is switched to the "
                    "default value: " + to_string(kDefaultMyNCBITimeoutMs));
        m_MyNCBITimeoutMs = kDefaultMyNCBITimeoutMs;
    }
}


unsigned long
SPubseqGatewaySettings::x_GetDataSize(const CNcbiRegistry &  registry,
                                      const string &  section,
                                      const string &  entry,
                                      unsigned long  default_val)
{
    CConfig                         conf(registry);
    const CConfig::TParamTree *     param_tree = conf.GetTree();
    const TPluginManagerParamTree * section_tree =
                                        param_tree->FindSubNode(section);

    if (!section_tree)
        return default_val;

    CConfig     c((CConfig::TParamTree*)section_tree, eNoOwnership);
    return c.GetDataSize("psg", entry, CConfig::eErr_NoThrow,
                         default_val);
}


size_t SPubseqGatewaySettings::GetProcessorMaxConcurrency(
                                            const CNcbiRegistry &   registry,
                                            const string &  processor_id)
{
    string                  section = processor_id + "_PROCESSOR";

    if (registry.HasEntry(section, "ProcessorMaxConcurrency")) {
        size_t      limit = registry.GetInt(section,
                                            "ProcessorMaxConcurrency",
                                            m_ProcessorMaxConcurrency);
        if (limit == 0) {
           PSG_WARNING("Invalid [" + section + "]/ProcessorMaxConcurrency value (" +
                       to_string(limit) + "). "
                       "The processor max concurrency must be greater than 0. "
                       "The processor max concurrency is reset to the "
                       "non-processor specific default value (" +
                       to_string(m_ProcessorMaxConcurrency) + ").");
            limit = m_ProcessorMaxConcurrency;
        }

        return limit;
    }

    // No processor specific value => server wide (or default)
    return m_ProcessorMaxConcurrency;
}
