namespace bar
{
    void function(int);
    void function(int) {}
}

int function(int, int, int);
void function(int, int);
void function(int, int)
{
    {
       bar::function(function(1,2,3));
    }

    {
        bar::function(2);
    }
}
