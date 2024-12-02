#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <time.h>

// Function to initialize ncurses
void init_ncurses() {
    initscr();               // Initialize ncurses mode
    cbreak();                // Disable line buffering
    noecho();                // Disable input echoing
    curs_set(0);             // Hide the cursor
    timeout(100);            // Set non-blocking input (100ms delay)
}

// Function to close ncurses
void close_ncurses() {
    endwin();                // End ncurses mode
}

// Function to get CPU usage
void get_cpu_usage(float* cpu_percent) {
    FILE *fp = fopen("/proc/stat", "r");
    if (fp == NULL) return;

    char line[256];
    unsigned long long int user, nice, system, idle, iowait, irq, softirq, steal;
    if (fgets(line, sizeof(line), fp) != NULL) {
        sscanf(line, "cpu  %llu %llu %llu %llu %llu %llu %llu %llu", 
               &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
    }
    fclose(fp);

    unsigned long long int total = user + nice + system + idle + iowait + irq + softirq + steal;
    unsigned long long int active = user + nice + system + irq + softirq + steal;
    *cpu_percent = (active * 100.0) / total;
}

// Function to get memory usage
void get_memory_usage(float* mem_used, float* mem_free) {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (fp == NULL) return;

    char line[256];
    unsigned long total, free, buffers, cached;
    if (fgets(line, sizeof(line), fp) != NULL) {
        sscanf(line, "MemTotal: %lu kB", &total);
    }
    if (fgets(line, sizeof(line), fp) != NULL) {
        sscanf(line, "MemFree: %lu kB", &free);
    }
    if (fgets(line, sizeof(line), fp) != NULL) {
        sscanf(line, "Buffers: %lu kB", &buffers);
    }
    if (fgets(line, sizeof(line), fp) != NULL) {
        sscanf(line, "Cached: %lu kB", &cached);
    }
    fclose(fp);

    *mem_free = free;
    *mem_used = total - free - buffers - cached;
}

// Function to get network usage
void get_network_usage(float* net_in, float* net_out) {
    FILE *fp = fopen("/proc/net/dev", "r");
    if (fp == NULL) return;

    char line[256];
    unsigned long long int recv, sent;
    while (fgets(line, sizeof(line), fp)) {
        // Match "eth0" or "wlan0" interface
        if (strstr(line, "eth0") != NULL || strstr(line, "wlan0") != NULL) {
            // Corrected sscanf format
            if (sscanf(line, " %*s %llu %*llu %*llu %*llu %*llu %*llu %*llu %*llu %llu", &recv, &sent) == 2) {
                *net_in = recv / 1024.0;  // Convert bytes to KB
                *net_out = sent / 1024.0; // Convert bytes to KB
            }
            break;
        }
    }
    fclose(fp);
}

// Function to get system info (like neofetch)
void get_system_info(char* distro, char* kernel, char* uptime) {
    // Distro info from /etc/os-release
    FILE *fp = fopen("/etc/os-release", "r");
    if (fp == NULL) {
        strcpy(distro, "Unknown Distro");
    } else {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, "PRETTY_NAME")) {
                sscanf(line, "PRETTY_NAME=\"%[^\"]", distro);
                break;
            }
        }
        fclose(fp);
    }

    // Kernel info using uname
    struct utsname uname_info;
    uname(&uname_info);  // Now works because we've included sys/utsname.h
    strcpy(kernel, uname_info.release);

    // Uptime using sysinfo
    struct sysinfo info;
    sysinfo(&info);
    sprintf(uptime, "%ld days, %ld hours, %ld minutes", info.uptime / 86400, (info.uptime % 86400) / 3600, ((info.uptime % 86400) % 3600) / 60);
}

// Function to display the data in ncurses windows
void display_data(WINDOW* cpu_win, WINDOW* mem_win, WINDOW* net_win, WINDOW* sys_win, WINDOW* clock_win) {
    float cpu_percent, mem_used, mem_free, net_in, net_out;
    char distro[256], kernel[256], uptime[256];
    char time_str[128];

    get_cpu_usage(&cpu_percent);
    get_memory_usage(&mem_used, &mem_free);
    get_network_usage(&net_in, &net_out);
    get_system_info(distro, kernel, uptime);

    // Get current time for clock window
    time_t raw_time;
    struct tm *time_info;
    time(&raw_time);
    time_info = localtime(&raw_time);
    strftime(time_str, sizeof(time_str), "%H:%M:%S", time_info);

    // Clear windows
    werase(cpu_win);
    werase(mem_win);
    werase(net_win);
    werase(sys_win);
    werase(clock_win);

    // Box the windows
    box(cpu_win, 0, 0);
    box(mem_win, 0, 0);
    box(net_win, 0, 0);
    box(sys_win, 0, 0);
    box(clock_win, 0, 0);

    // Display Clock
    mvwprintw(clock_win, 1, 1, "Time: %s", time_str);

    // System Info (inside box)
    mvwprintw(sys_win, 1, 1, "Distro: %s", distro);
    mvwprintw(sys_win, 2, 1, "Kernel: %s", kernel);
    mvwprintw(sys_win, 3, 1, "Uptime: %s", uptime);

    // CPU usage display
    mvwprintw(cpu_win, 1, 1, "CPU Usage: %.2f%%", cpu_percent);

    // Memory usage display
    mvwprintw(mem_win, 1, 1, "Memory Usage: %.2f MB Used / %.2f MB Free", mem_used / 1024.0, mem_free / 1024.0);

    // Network usage display
    mvwprintw(net_win, 1, 1, "Network In: %.2f KB, Out: %.2f KB", net_in, net_out);

    // Refresh windows
    wrefresh(cpu_win);
    wrefresh(mem_win);
    wrefresh(net_win);
    wrefresh(sys_win);
    wrefresh(clock_win);
}

int main() {
    // Initialize ncurses
    init_ncurses();

    // Create windows for each section
    int height = 5, width = COLS;
    WINDOW* sys_win = newwin(5, width, 0, 0);
    WINDOW* cpu_win = newwin(height, width, 5, 0);
    WINDOW* mem_win = newwin(height, width, 10, 0);
    WINDOW* net_win = newwin(height, width, 15, 0);
    WINDOW* clock_win = newwin(3, 20, 0, COLS - 20);  // Clock window at top right corner

    // Main loop to update system data
    while (1) {
        display_data(cpu_win, mem_win, net_win, sys_win, clock_win);
        usleep(100000);  // Sleep for 100ms (adjust for desired refresh rate)

        // Exit on pressing 'q'
        int ch = getch();
        if (ch == 'q') break;
    }

    // Close ncurses
    close_ncurses();
    return 0;
}

