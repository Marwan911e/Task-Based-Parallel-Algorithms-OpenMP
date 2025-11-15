#include <stdio.h>
#include <stdbool.h>
#include <omp.h>

int main() {
    // Input variables
    int A, B, C, D;
    bool E, F;
    
    // Intermediate results for each comparison
    bool result1, result2, result3;
    
    // Final result
    bool Y;
    
    // Get input values from user
    printf("Enter value for A: ");
    scanf("%d", &A);
    
    printf("Enter value for B: ");
    scanf("%d", &B);
    
    printf("Enter value for C: ");
    scanf("%d", &C);
    
    printf("Enter value for D: ");
    scanf("%d", &D);
    
    printf("Enter value for E (0 or 1): ");
    int temp_E;
    scanf("%d", &temp_E);
    E = (temp_E != 0);
    
    printf("Enter value for F (0 or 1): ");
    int temp_F;
    scanf("%d", &temp_F);
    F = (temp_F != 0);
    
    printf("\n----- Evaluating: Y = (A == B) AND (C != D) AND (E OR F) -----\n\n");
    
    // Parallel region using sections to assign each comparison to a separate thread
    #pragma omp parallel sections num_threads(3)
    {
        #pragma omp section
        {
            // Thread 1: Evaluate (A == B)
            result1 = (A == B);
            printf("Thread %d: Evaluating (A == B) => (%d == %d) = %s\n", 
                   omp_get_thread_num(), A, B, result1 ? "true" : "false");
        }
        
        #pragma omp section
        {
            // Thread 2: Evaluate (C != D)
            result2 = (C != D);
            printf("Thread %d: Evaluating (C != D) => (%d != %d) = %s\n", 
                   omp_get_thread_num(), C, D, result2 ? "true" : "false");
        }
        
        #pragma omp section
        {
            // Thread 3: Evaluate (E OR F)
            result3 = (E || F);
            printf("Thread %d: Evaluating (E OR F) => (%s OR %s) = %s\n", 
                   omp_get_thread_num(), 
                   E ? "true" : "false", 
                   F ? "true" : "false", 
                   result3 ? "true" : "false");
        }
    }
    
    // Combine the results (sequential after parallel region)
    Y = result1 && result2 && result3;
    
    printf("\n----- Final Result -----\n");
    printf("Y = (%s) AND (%s) AND (%s) = %s\n", 
           result1 ? "true" : "false",
           result2 ? "true" : "false",
           result3 ? "true" : "false",
           Y ? "true" : "false");
    
    return 0;
}

