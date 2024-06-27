//
// Created by caogaoshuai on 2024/2/7.
//

#include "hdns_api.h"

#if defined(__APPLE__)

#include <stdio.h>
#include <mach/mach.h>
#include <mach/task_info.h>
#include <unistd.h>

int64_t get_memory_usage_on_mac();

#elif defined(__linux__)
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int64_t get_memory_usage_on_linux();

#elif defined(_WIN32)
#include <stdio.h>
#include <windows.h>
#include <psapi.h>

int64_t get_memory_usage_on_windows();

#else

int64_t get_memory_usage_on_other_platform();

#endif

#define MOCK_BUSINESS_HOST        "www.taobao.com"
#define MOCK_HTTPDNS_ACCOUNT      "139450"
#define MOCK_HTTPDNS_SECRET       NULL
#define API_QUERY_TIMES           200

int64_t get_memory_usage();


#define MEASURE_TIME(func, ...) \
    do { \
        apr_time_t start = apr_time_now(); \
        func(__VA_ARGS__);      \
        apr_time_t end = apr_time_now();   \
        g_metric.api_time_cost_us[fn_index] += (end - start); \
        g_metric.memeroy_usage_bytes[fn_index] = get_memory_usage(); \
        hdns_list_cleanup(results);        \
        fn_index++;                \
    } while (0)

typedef struct {
    int64_t total_times;
    int64_t memeroy_usage_bytes[4];
    int64_t api_time_cost_us[4];
} hdns_metric_t;


hdns_metric_t g_metric = {
        .total_times=0,
};


int main(int argc, char *argv[]) {
    hdns_to_void_p(argv);
    hdns_to_int(argc);
    // 1. HTTPDNS SDK 环境初始化
    if (hdns_sdk_init() != HDNS_OK) {
        goto cleanup;
    }
    // 2. 创建客户端（可复用）
    hdns_client_t *client = hdns_client_create(MOCK_HTTPDNS_ACCOUNT, MOCK_HTTPDNS_SECRET);
    if (client == NULL) {
        goto cleanup;
    }
    // 3. 配置客户端（可选）
    hdns_client_set_timeout(client, 3000);
    hdns_client_set_using_cache(client, true);
    hdns_client_set_using_https(client, true);
    hdns_client_set_using_sign(client, true);
    hdns_client_set_retry_times(client, 1);
    hdns_client_set_region(client, "global");
    hdns_client_set_schedule_center_region(client, "cn");

    // 4. 启动客户端
    hdns_client_start(client);

    hdns_list_head_t *results = NULL;
    hdns_list_head_t *hosts = hdns_list_create();
    hdns_list_add_str(hosts, "www.taobao.com");
    hdns_list_add_str(hosts, "www.aliyun.com");
    hdns_list_add_str(hosts, "www.tmall.com");
    // 5. 接口调用
    for (int i = 0; i < API_QUERY_TIMES; i++) {
        g_metric.total_times++;
        int32_t fn_index = 0;
        MEASURE_TIME(hdns_get_result_for_host_sync_without_cache, client,
                     MOCK_BUSINESS_HOST,
                     HDNS_QUERY_AUTO,
                     NULL, &results);
        MEASURE_TIME(hdns_get_result_for_host_sync_with_cache, client,
                     MOCK_BUSINESS_HOST,
                     HDNS_QUERY_AUTO,
                     NULL, &results);
        MEASURE_TIME(hdns_get_results_for_hosts_sync_without_cache, client,
                     hosts,
                     HDNS_QUERY_AUTO,
                     NULL, &results);
        MEASURE_TIME(hdns_get_results_for_hosts_sync_with_cache, client,
                     hosts,
                     HDNS_QUERY_AUTO,
                     NULL, &results);
    }

    // 6. 客户端释放
    hdns_list_cleanup(hosts);
    hdns_client_cleanup(client);
    cleanup:
    // 7. SDK环境释放
    hdns_sdk_cleanup();

    // 8. 指标打印
    char *api_name[] = {"hdns_get_result_for_host_sync_without_cache",
                        "hdns_get_result_for_host_sync_with_cache",
                        "hdns_get_results_for_hosts_sync_without_cache",
                        "hdns_get_results_for_hosts_sync_with_cache"};
    printf("Benchmark Summary: \n");
    printf("API Call Times: %d\n", g_metric.total_times);
    printf("API Memory Usage: \n");
    for (int i = 0; i < 4; i++) {
        printf("\t%s: %d KB\n", api_name[i], g_metric.memeroy_usage_bytes[i] / 1024);
    }
    printf("API Time Cost: \n");
    for (int i = 0; i < 4; i++) {
        printf("\t%s: %d ms\n", api_name[i], g_metric.api_time_cost_us[i] / g_metric.total_times / 1000);
    }
    return 0;
}


int64_t get_memory_usage() {
#if defined(__APPLE__)
    return get_memory_usage_on_mac();
#elif defined(__linux__)
    return get_memory_usage_on_linux();
#elif defined(_WIN32)
    return get_memory_usage_on_windows();
#else
    return get_memory_usage_on_other_platform();
#endif
}

#if defined(__APPLE__)

int64_t get_memory_usage_on_mac() {
    task_t task = MACH_PORT_NULL;
    task_basic_info_data_t info;
    mach_msg_type_number_t info_count = TASK_BASIC_INFO_COUNT;

    if (task_for_pid(mach_task_self(), getpid(), &task) != KERN_SUCCESS) {
        fprintf(stderr, "Failed to get task for PID.\n");
        return 0;
    }
    if (task_info(task, TASK_BASIC_INFO, (task_info_t) &info, &info_count) != KERN_SUCCESS) {
        fprintf(stderr, "Failed to get task info.\n");
        return 0;
    }
    return info.resident_size;
}

#elif defined(__linux__)

int64_t get_memory_usage_on_linux() {
    hdns_pool_new(pool);
    apr_file_t *file;
    apr_status_t rv;
    char line[256];
    const char *fname = "/proc/self/statm";

    rv = apr_file_open(&file, fname, APR_READ, APR_OS_DEFAULT, pool);
    if (rv != APR_SUCCESS) {
        char errbuf[256];
        apr_strerror(rv, errbuf, sizeof(errbuf));
        fprintf(stderr, "Cannot open file %s: %s\n", fname, errbuf);
        hdns_pool_destroy(pool);
        return 0;
    }
    if (apr_file_gets(line, sizeof(line), file) == APR_SUCCESS) {
        long page_size = sysconf(_SC_PAGESIZE);
        unsigned long vm_pages, rss_pages;
        sscanf(line, "%lu %*s %*s %*s %*s %*s %lu", &vm_pages, &rss_pages);
        apr_file_close(file);
        hdns_pool_destroy(pool);
        return rss_pages * page_size;
    }
   return 0;
}

#elif defined(_WIN32)
int64_t get_memory_usage_on_windows() {
    PROCESS_MEMORY_COUNTERS_EX pmc;
    HANDLE hProcess = GetCurrentProcess();
    int64_t mem_size = 0L;
    if (GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
       mem_size = pmc.WorkingSetSize;
    } else {
        fprintf(stderr, "Failed to get process memory info\n");
        mem_size= 0L;
    }
    CloseHandle(hProcess);
    return mem_size;
}
#else
int64_t get_memory_usage_on_other_platform() {
    fprintf(stderr, "Unkown OS platform\n");
    return 0;
}
#endif