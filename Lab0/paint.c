/* File: paint.c 
   Author:Borui Liu
   
*/
#define CAN_COVERAGE 200

#include <stdio.h>
#include <math.h>

/* If you do not use the Makefile provided and use gcc,
 * and if you continue to use the math.h library, you
 * will need to include -lm in your gcc compile statement
 * to load the math library */

/* Optional functions, uncomment the next two lines
 * if you want to create these functions after main: */
//float readDimension(const char* name);
//float calcArea(float width, float height, float depth);

int main(int argc, char *argv[]){

    float width, height, depth;
    float area;
    int num;

    /* Prompt user for pool dimensions */
    printf("Enter pool width: ");
    scanf("%f", &width);

    printf("Enter pool height: ");
    scanf("%f", &height);

    printf("Enter pool depth: ");
    scanf("%f", &depth);

    area = 2 * width * height + 2 * width * depth + 2 * height * depth;

    num = (int)ceil(area / CAN_COVERAGE);

    printf("Total area to cover: %.2f square feet\n", area);
    printf("Number of paint cans required: %d\n", num);

    return 0;
}
