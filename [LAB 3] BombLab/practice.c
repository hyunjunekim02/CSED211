#include <stdio.h>

int practice_function(int, int);

int main()
{
	int x, y;
	printf("Which one is larger? : ");
	scanf("%d %d", &x, &y);
	practice_function(x, y);
}

int practice_function(int x, int y)
{
	if (x > y)
	{
		printf("First is larger!");
	}
	else if (x < y)
	{
		printf("Second is larger!");
	}
	else
	{
		printf("both are same!");
	}
}

