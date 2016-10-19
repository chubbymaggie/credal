int main(int argc, char *argv[])
{
    char *p, *q;
    char *crash;
    char buff[15];

    crash = buff;

    // strcpy(buff, argv[1]);
    for (p=buff,q=argv[1];(*q);p++,q++)
	(*p) = (*q);

    (*crash) = (*crash);
    return 0;
}           
