#include "types.h"
#include "stat.h"
#include "user.h"
#include "pstat.h"

// Default ticket configuration - used if no arguments provided
#define DEFAULT_P1_TICKETS 30  // High priority process
#define DEFAULT_P2_TICKETS 20  // Medium priority process  
#define DEFAULT_P3_TICKETS 10  // Low priority process
#define RUNTIME 7000   // Test duration in ticks
#define MAX_PROCESSES 32  // Maximum number of processes (increased from 16)

// Global variables for dynamic configuration
int num_processes = 3;
int process_tickets[MAX_PROCESSES];
int process_pids[MAX_PROCESSES];

// Function declarations
int simple_str_copy(char* dest, char* src);
int simple_int_to_str(char* dest, char* prefix, int num);
char* get_live_stats(void);
void print_dynamic_banner(void);
int str_to_int(char* str);

// Simple string to integer conversion
int str_to_int(char* str) {
    int result = 0;
    int i = 0;
    while (str[i] >= '0' && str[i] <= '9') {
        result = result * 10 + (str[i] - '0');
        i++;
    }
    return result;
}

// Utility function to get total tickets dynamically
int get_total_tickets(void) {
    int total = 0;
    for (int i = 0; i < num_processes; i++) {
        total += process_tickets[i];
    }
    return total;
}

// Print a dynamic banner with current configuration
void print_dynamic_banner(void) {
    int total = get_total_tickets();
    printf(1, "\n");
    printf(1, "==================================================\n");
    printf(1, "       XV6 LOTTERY SCHEDULER DEMONSTRATION        \n");
    printf(1, "             Configuration: ");
    for (int i = 0; i < num_processes; i++) {
        printf(1, "%d", process_tickets[i]);
        if (i < num_processes - 1) printf(1, ":");
    }
    printf(1, "             \n");
    printf(1, "             Total Tickets: %d                   \n", total);
    printf(1, "==================================================\n");
    printf(1, "\n");
}

// Infinite loop that can be interrupted
void infinite_work()
{
    volatile long sum = 0;
    for (volatile long i = 0; ; i++) {
        sum += (i * i) % 997;
        // No yield - let the scheduler preempt us
    }
}

void print_header()
{
    print_dynamic_banner();
}

void print_test_setup()
{
    int total_tickets = get_total_tickets();
    
    printf(1, "🎲 TEST CONFIGURATION:\n");
    for (int i = 0; i < num_processes; i++) {
        int expected = (process_tickets[i] * 100) / total_tickets;
        printf(1, "   ├─ Process P%d: %d tickets\n", i+1, process_tickets[i]);
    }
    printf(1, "   Total Pool: %d tickets\n", total_tickets);
    printf(1, "\n");
    printf(1, "📊 EXPECTED ALLOCATION (Proportional Fair Share):\n");
    for (int i = 0; i < num_processes; i++) {
        int expected = (process_tickets[i] * 100) / total_tickets;
        printf(1, "   ├─ P%d: %d.%d%% of CPU time (%d/%d tickets)\n", 
               i+1, expected, ((process_tickets[i] * 1000) / total_tickets) % 10, 
               process_tickets[i], total_tickets);
    }
    printf(1, "\n");
    printf(1, "   Ratio: ");
    for (int i = 0; i < num_processes; i++) {
        printf(1, "%d", process_tickets[i]);
        if (i < num_processes - 1) printf(1, ":");
    }
    printf(1, "\n\n");
}

void print_progress_bar(int current, int total, char* label, char* extra_info)
{
    int bar_width = 40;
    int progress = (current * bar_width) / total;
    int percentage = (current * 100) / total;
    
    printf(1, "\r%s [", label);  // \r to overwrite current line
    for (int i = 0; i < bar_width; i++) {
        if (i < progress) printf(1, "█");
        else printf(1, "░");
    }
    printf(1, "] %d%%", percentage);
    
    // Add extra info if provided
    if (extra_info) {
        printf(1, " %s", extra_info);
    }
    
    // Clear to end of line to remove any leftover text
    printf(1, "                                        ");
    
    // Only add newline when complete
    if (percentage >= 100) {
        printf(1, "\n");
    }
}

char stats_buffer[200];  // Global buffer for stats string

char* get_live_stats(void)
{
    struct pstat st;
    if (getpinfo(&st) == 0) {
        int process_ticks[MAX_PROCESSES];
        int total = 0;
        
        // Initialize ticks array
        for (int i = 0; i < num_processes; i++) {
            process_ticks[i] = 0;
        }
        
        // Find ticks for each process
        for (int i = 0; i < NPROC; i++) {
            if (st.inuse[i]) {
                for (int j = 0; j < num_processes; j++) {
                    if (st.pid[i] == process_pids[j]) {
                        process_ticks[j] = st.ticks[i];
                        total += st.ticks[i];
                        break;
                    }
                }
            }
        }
        
        if (total > 0) {
            // Build the stats string manually
            char* ptr = stats_buffer;
            
            for (int i = 0; i < num_processes; i++) {
                int pct = (process_ticks[i] * 100) / total;
                
                // Add process stats
                ptr += simple_int_to_str(ptr, "P", i+1);
                ptr += simple_str_copy(ptr, "=");
                ptr += simple_int_to_str(ptr, "", process_ticks[i]);
                ptr += simple_int_to_str(ptr, " (", pct);
                ptr += simple_str_copy(ptr, "%)");
                
                if (i < num_processes - 1) {
                    ptr += simple_str_copy(ptr, ", ");
                }
            }
            
            return stats_buffer;
        }
    }
    return 0;
}

int simple_str_copy(char* dest, char* src) {
    int count = 0;
    while (*src) {
        *dest++ = *src++;
        count++;
    }
    return count;
}

int simple_int_to_str(char* dest, char* prefix, int num) {
    int count = 0;
    
    // Copy prefix
    while (*prefix) {
        *dest++ = *prefix++;
        count++;
    }
    
    // Convert number to string
    if (num == 0) {
        *dest++ = '0';
        count++;
    } else {
        char temp[20];
        int i = 0;
        while (num > 0) {
            temp[i++] = '0' + (num % 10);
            num /= 10;
        }
        while (--i >= 0) {
            *dest++ = temp[i];
            count++;
        }
    }
    return count;
}

void print_visual_results(int process_ticks[], int total_ticks)
{
    printf(1, "\n");
    printf(1, "==================================================\n");
    printf(1, "               LOTTERY TEST RESULTS               \n");
    printf(1, "==================================================\n");
    printf(1, "\n");
    
    // Print numerical results - centered
    printf(1, "                📈 SCHEDULING STATISTICS\n");
    printf(1, "   ┌─────────────────────────────────────────────┐\n");
    printf(1, "   │ Process │ Tickets │  Ticks  │ Percentage │\n");
    printf(1, "   ├─────────────────────────────────────────────┤\n");
    
    for (int i = 0; i < num_processes; i++) {
        printf(1, "   │   P%d    │   %d    │  %d   │    %d%%     │\n", 
               i+1, process_tickets[i], process_ticks[i], (process_ticks[i] * 100) / total_ticks);
    }
    
    printf(1, "   └─────────────────────────────────────────────┘\n");
    printf(1, "                    Total Ticks: %d\n", total_ticks);
    printf(1, "\n");
    
    // Visual representation with bars - centered
    printf(1, "             📊 VISUAL CPU TIME DISTRIBUTION\n");
    printf(1, "\n");
    
    // Calculate bar lengths dynamically (max 40 chars for better centering)
    int max_bar = 40;
    
    for (int i = 0; i < num_processes; i++) {
        int bar_length = (process_ticks[i] * max_bar) / total_ticks;
        int pct = (process_ticks[i] * 100) / total_ticks;
        
        printf(1, "       P%d (%d tickets): ", i+1, process_tickets[i]);
        for (int j = 0; j < bar_length; j++) printf(1, "█");
        for (int j = bar_length; j < max_bar; j++) printf(1, "░");
        printf(1, " %d%%\n", pct);
    }
    
    printf(1, "\n");
    printf(1, "       Scale: ");
    for (int i = 0; i <= max_bar; i += 10) printf(1, "|");
    printf(1, "\n              ");
    for (int i = 0; i <= max_bar; i += 10) {
        int scale_val = (i * 100) / max_bar;
        printf(1, "%d    ", scale_val);
    }
    printf(1, "\n");
}

void print_accuracy_analysis(int process_ticks[], int total_ticks)
{
    printf(1, "\n");
    printf(1, "                   🎯 ACCURACY ANALYSIS\n");
    printf(1, "\n");
    
    // Calculate expected percentages dynamically
    int total_tickets = get_total_tickets();
    
    printf(1, "   ┌─────────────────────────────────────────────┐\n");
    printf(1, "   │ Process │ Expected │ Actual │ Deviation │\n");
    printf(1, "   ├─────────────────────────────────────────────┤\n");
    
    int total_deviation = 0;
    for (int i = 0; i < num_processes; i++) {
        int expected = (process_tickets[i] * 100) / total_tickets;
        int actual = (process_ticks[i] * 100) / total_ticks;
        int deviation = actual - expected;
        
        printf(1, "   │   P%d    │   %d%%    │  %d%%   │    ", i+1, expected, actual);
        if (deviation >= 0) printf(1, "+%d%%    │\n", deviation);
        else printf(1, "%d%%    │\n", deviation);
        
        // Add absolute deviation to total
        if (deviation < 0) total_deviation += -deviation;
        else total_deviation += deviation;
    }
    
    printf(1, "   └─────────────────────────────────────────────┘\n");
    printf(1, "\n");
    
    // Calculate overall accuracy
    int accuracy = 100 - (total_deviation / num_processes);
    
    printf(1, "           🏆 LOTTERY SCHEDULER ACCURACY: %d%%\n", accuracy);
    printf(1, "\n");
    
    if (accuracy >= 90) {
        printf(1, "        ✅ EXCELLENT: Lottery scheduler working perfectly!\n");
    } else if (accuracy >= 80) {
        printf(1, "            ✅ GOOD: Lottery scheduler working well!\n");
    } else if (accuracy >= 70) {
        printf(1, "         ⚠️  FAIR: Lottery scheduler needs improvement.\n");
    } else {
        printf(1, "            ❌ POOR: Lottery scheduler has issues.\n");
    }
    
    printf(1, "\n");
    printf(1, "                📊 PROPORTIONAL RATIOS\n");
    
    // Calculate ratios relative to the smallest value for better comparison
    int min_tickets = process_tickets[0];
    int min_ticks = process_ticks[0];
    
    for (int i = 1; i < num_processes; i++) {
        if (process_tickets[i] < min_tickets) min_tickets = process_tickets[i];
        if (process_ticks[i] < min_ticks) min_ticks = process_ticks[i];
    }
    
    if (min_ticks > 0) {
        printf(1, "             Expected: ");
        for (int i = 0; i < num_processes; i++) {
            int ratio = (process_tickets[i] * 10) / min_tickets;
            printf(1, "%d.%d", ratio / 10, ratio % 10);
            if (i < num_processes - 1) printf(1, " : ");
        }
        printf(1, "\n");
        
        printf(1, "             Actual  : ");
        for (int i = 0; i < num_processes; i++) {
            int ratio = (process_ticks[i] * 10) / min_ticks;
            printf(1, "%d.%d", ratio / 10, ratio % 10);
            if (i < num_processes - 1) printf(1, " : ");
        }
        printf(1, "\n");
    }
    
    printf(1, "\n");
    printf(1, "              📝 CONFIGURATION SUMMARY\n");
    printf(1, "           Ticket Ratio: ");
    for (int i = 0; i < num_processes; i++) {
        printf(1, "%d", process_tickets[i]);
        if (i < num_processes - 1) printf(1, ":");
    }
    printf(1, "\n");
    printf(1, "           Test Duration: %d ticks\n", RUNTIME);
    printf(1, "           Total Samples: %d ticks\n", total_ticks);
    printf(1, "\n");
    printf(1, "==================================================\n");
}

int
main(int argc, char *argv[])
{
    printf(1, "\n");
    
    // Parse command line arguments or use defaults
    if (argc > 1) {
        num_processes = argc - 1;
        
        if (num_processes > MAX_PROCESSES) {
            printf(2, "ERROR: Too many processes (max %d, got %d)\n", MAX_PROCESSES, num_processes);
            printf(2, "Usage: %s [ticket1] [ticket2] ... [ticketN]\n", argv[0]);
            printf(2, "Example: %s 10 20 15 5\n", argv[0]);
            exit();
        }
        
        printf(1, "Parsing %d processes from command line:\n", num_processes);
        for (int i = 0; i < num_processes; i++) {
            process_tickets[i] = str_to_int(argv[i + 1]);
            printf(1, "  Process %d: %d tickets (from arg '%s')\n", i+1, process_tickets[i], argv[i + 1]);
            if (process_tickets[i] <= 0) {
                printf(2, "ERROR: Invalid ticket count: %s\n", argv[i + 1]);
                exit();
            }
        }
        printf(1, "\n");
    } else {
        // Use default configuration
        printf(1, "No arguments provided, using default configuration\n");
        num_processes = 3;
        process_tickets[0] = DEFAULT_P1_TICKETS;
        process_tickets[1] = DEFAULT_P2_TICKETS;
        process_tickets[2] = DEFAULT_P3_TICKETS;
    }

    print_header();
    print_test_setup();
    
    printf(1, "🚀 STARTING PROCESSES...\n");
    
    // Create child processes dynamically
    for (int i = 0; i < num_processes; i++) {
        int pid = fork();
        if (pid == 0) {
            settickets(process_tickets[i]);
            printf(1, "   ✓ P%d (PID %d) started with %d tickets\n", i+1, getpid(), process_tickets[i]);
            infinite_work();
            exit();
        }
        process_pids[i] = pid;
    }

    // Parent process - minimal tickets
    settickets(1);
    printf(1, "   ✓ Parent monitoring with 1 ticket\n");
    printf(1, "\n");
    
    sleep(100); // Give children time to start
    
    printf(1, "⏱️  RUNNING TEST FOR %d TICKS...\n", RUNTIME);
    printf(1, "\n");
    
    // Show progress during test with live stats
    for (int i = 0; i < 10; i++) {
        sleep(RUNTIME / 45);
        char* stats = get_live_stats();
        print_progress_bar(i + 1, 10, "Progress", stats);
    }
    
    printf(1, "\n⏹️  TEST COMPLETE. COLLECTING RESULTS...\n");
    
    // Kill all children
    for (int i = 0; i < num_processes; i++) {
        kill(process_pids[i]);
    }
    
    // Get final statistics  
    struct pstat final_st;
    if (getpinfo(&final_st) < 0) {
        printf(2, "ERROR: getpinfo failed\n");
        exit();
    }
    
    // Wait for children to exit
    for (int i = 0; i < num_processes; i++) {
        wait();
    }

    int process_ticks[MAX_PROCESSES];
    int total_ticks = 0;
    
    // Initialize ticks array
    for (int i = 0; i < num_processes; i++) {
        process_ticks[i] = 0;
    }
    
    // Extract final tick counts
    for (int i = 0; i < NPROC; i++) {
        if (final_st.inuse[i]) {
            for (int j = 0; j < num_processes; j++) {
                if (final_st.pid[i] == process_pids[j]) {
                    process_ticks[j] = final_st.ticks[i];
                    total_ticks += final_st.ticks[i];
                    break;
                }
            }
        }
    }
    
    print_visual_results(process_ticks, total_ticks);
    print_accuracy_analysis(process_ticks, total_ticks);

    exit();
}