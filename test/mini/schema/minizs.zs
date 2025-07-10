package minizs;

struct Inner
{
    string key; 
    uint8 value;
};

struct Outer(uint8 numOfInners)
{
    Inner inner[numOfInners];
};

struct MostOuter
{
    uint8 numOfInner;
    Outer(numOfInner) outer;
};
