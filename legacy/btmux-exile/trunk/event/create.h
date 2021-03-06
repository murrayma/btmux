
#ifndef CREATE_H
#define CREATE_H

#define Create(a,b,c) \
if (!((a) = ( b * ) calloc(sizeof( b ), c ) )) \
{ printf ("Unable to malloc!\n"); exit(1); }

#define MyReCreate(a,b,c) \
if (!((a) = ( b * ) realloc((void *) a, sizeof( b ) * (c) ) )) \
{ printf ("Unable to realloc!\n"); exit(1); }

#define ReCreate(a,b,c) \
if (a) { MyReCreate(a,b,c); } else { Create(a,b,c); }

#define Free(a) if (a) {free(a);a=0;}

#endif				/* CREATE_H */
