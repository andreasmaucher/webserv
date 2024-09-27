#include <iostream>

int main(int ac, char **av)
{
    (void)ac;
    if (ac < 1 || ac > 2)
    {
        std::cout << "Invalid number of arguments" << std::endl;
        return (1);
    }
}