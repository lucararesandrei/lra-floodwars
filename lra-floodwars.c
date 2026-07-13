/*
    Nume program   : lra-floodwars.c
    Nume concurent : Luca Rares Andrei
    E-mail         : hidden
*/

// lra-floodwars-v10

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define MARGINE -1
#define INF 2000000000

int n, m;
char caractere[5] = {'.', '#', '+', '@', '*'};

// negamax, killer moves si iterative deepening
int nivel_max, timp_verif, noduri;
long long last_timp;
char kill[100];

// cozi bfs globale (circulare)
#define QMAX 32768
int ql[QMAX], qc[QMAX], prim, ultim;

// cu top la temp (vizitat deja la dfs) nu mai pierd timp pentru memset
#define NMAX 50
int temp[NMAX+2][NMAX+2], top = 0;

// distante "voronoi" (sau problema "traversare")
int vizdist1[NMAX+2][NMAX+2], vizdist2[NMAX+2][NMAX+2], 
    dist1[NMAX+2][NMAX+2], dist2[NMAX+2][NMAX+2];

// toate celulele jucatorilor, intr-o stiva
int travtop = 0;
int l1[2500], c1[2500], top1, 
    l2[2500], c2[2500], top2;

// undo de modificari (op pe biti)
unsigned int modif[200000]; 
int modiftop = 0;

// graf de componente
int compid[NMAX+2][NMAX+2], comparie[2501], 
    compatins1[2501], compatins2[2501];

// tabele de transpozitie cu zobrist hashing (xor)
#define TSIZE 1048576
unsigned long long zob[NMAX+2][NMAX+2][256],
    zobjuc, zobcur;
unsigned long long tt[TSIZE];
int ttadancime[TSIZE], ttscor[TSIZE], ttid[TSIZE];
char ttmutare[TSIZE];

//--------------------------------------//

// random number generator mai rapid decat rand(), folosind op pe biti
unsigned long long randseed = 321654987ULL;
unsigned long long rand64() {
    randseed ^= randseed << 13;
    randseed ^= randseed >> 7;
    randseed ^= randseed << 17;
    return randseed;
}

// valori aleatorii pentru zobrist hashing
void zobinit() {
    int i, j, c;
    for ( i = 0; i <= NMAX+1; i++ ) 
        for ( j = 0; j <= NMAX+1; j++ )
            for ( c = 0; c < 256; c++ )
                zob[i][j][c] = rand64();
    zobjuc = rand64();
}

//--------------------------------------//

// verificarea timpului pentru iterative deepening
static inline long long getTime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec*1000000LL+tv.tv_usec;
}

// marcarea celulelor curente ale fiecarui jucator
// totodata, punem in stivele l1, c1, l2, c2 pozitiile celulelor respective
void flood_viz(int l, int c, char ch, char jucator, char tabla[NMAX+2][NMAX+2], 
                char viz[NMAX+2][NMAX+2], int *culori) {
    viz[l][c] = jucator;
    if ( jucator == 1 )  {
        l1[top1] = l;
        c1[top1++] = c;
    } else {
        l2[top2] = l;
        c2[top2++] = c;
    }
    temp[l][c] = top;
     
    if ( culori ) {
        if ( viz[l+1][c] == 0 && tabla[l+1][c] != 'x' && tabla[l+1][c] != ch )
            culori[(int)tabla[l+1][c]]++;
        if ( viz[l-1][c] == 0 && tabla[l-1][c] != 'x' && tabla[l-1][c] != ch ) 
            culori[(int)tabla[l-1][c]]++;
        if ( viz[l][c+1] == 0 && tabla[l][c+1] != 'x' && tabla[l][c+1] != ch )
            culori[(int)tabla[l][c+1]]++;
        if ( viz[l][c-1] == 0 && tabla[l][c-1] != 'x' && tabla[l][c-1] != ch )
            culori[(int)tabla[l][c-1]]++;
    }

    if ( temp[l+1][c] != top && tabla[l+1][c] == ch )
        flood_viz(l+1, c, ch, jucator, tabla, viz, culori);

    if ( temp[l-1][c] != top && tabla[l-1][c] == ch ) 
        flood_viz(l-1, c, ch, jucator, tabla, viz, culori);

    if ( temp[l][c+1] != top && tabla[l][c+1] == ch )
        flood_viz(l, c+1, ch, jucator, tabla, viz, culori);

    if ( temp[l][c-1] != top && tabla[l][c-1] == ch )
        flood_viz(l, c-1, ch, jucator, tabla, viz, culori);

}

// abs() doar ca e al meu
int modul(int x) {
    if (x < 0)
        x = -x;
    return x;
}

// facem modificarea dorita
// totodata, il notam in stiva de modificari, sa nu pierdem timp cu doua table separate
void flood_update(int l, int c, char ch_nou, char jucator, char tabla[NMAX+2][NMAX+2], 
                    char viz[NMAX+2][NMAX+2]) {

    unsigned char vechi = (unsigned char)tabla[l][c];
    modif[modiftop++] = (l << 16) | (c << 8) | (unsigned char)tabla[l][c];
    temp[l][c] = top;
    tabla[l][c] = ch_nou;
    
    zobcur ^= zob[l][c][vechi] ^ zob[l][c][(unsigned char)ch_nou];

    if ( temp[l+1][c] != top && viz[l+1][c] == jucator )
        flood_update(l+1, c, ch_nou, jucator, tabla, viz);
    
    if ( temp[l-1][c] != top && viz[l-1][c] == jucator )
        flood_update(l-1, c, ch_nou, jucator, tabla, viz);

    if ( temp[l][c+1] != top && viz[l][c+1] == jucator )
        flood_update(l, c+1, ch_nou, jucator, tabla, viz);

    if ( temp[l][c-1] != top && viz[l][c-1] == jucator )
        flood_update(l, c-1, ch_nou, jucator, tabla, viz);

}

//--------------------------------------//

// distante "voronoi" (problema "traversare")
int dl[4] = {1, -1, 0, 0}, dc[4] = {0, 0, 1, -1};
void traversare(char tabla[NMAX+2][NMAX+2], int dist[NMAX+2][NMAX+2], int viz[NMAX+2][NMAX+2]) {
    int l, c, tmpl, tmpc, cost;
    int d;
    while ( prim != ultim ) {
        l = ql[prim]; c = qc[prim];
        prim = (prim+1)&(QMAX-1);

        for ( d = 0; d < 4; d++ ) {
            tmpl = l+dl[d]; tmpc = c+dc[d];
            if ( tabla[tmpl][tmpc] != 'x' ) {
                cost = 1 - ( tabla[tmpl][tmpc] == tabla[l][c] );
                if ( viz[tmpl][tmpc] != travtop || dist[l][c] + cost < dist[tmpl][tmpc] ) {
                    dist[tmpl][tmpc] = dist[l][c] + cost;
                    viz[tmpl][tmpc] = travtop;
                    if ( cost == 0 ) {
                        prim = (prim-1+QMAX)&(QMAX-1);
                        ql[prim] = tmpl; qc[prim] = tmpc;
                    } else {
                        ql[ultim] = tmpl; qc[ultim] = tmpc;
                        ultim = (ultim+1)&(QMAX-1);
                    }
                }
            }
        }
    }
}

// evaluare statica v9 - distante voronoi, graf de componente si mici chestii
int eval(char tabla[NMAX+2][NMAX+2], char viz[NMAX+2][NMAX+2], char juc) {
    int l, c, d, sc1, sc2, ar1, ar2, tmpl, tmpc, i,
        vor1, vor2, av1, av2, enclava1, enclava2, d1, d2,
        p, u, nl, nc;

    enclava1 = enclava2 = vor1 = vor2 = av1 = av2 = 0;

    travtop++;
    ar1 = top1; ar2 = top2;

    prim = ultim = 0;
    for ( i = 0; i < top1; i++ ) {
        l = l1[i]; c = c1[i];
        vizdist1[l][c] = travtop;
        dist1[l][c] = 0;
        ql[ultim] = l; qc[ultim] = c;
        ultim = (ultim+1)&(QMAX-1);
    }
    traversare(tabla, dist1, vizdist1);

    prim = ultim = 0;
    for ( i = 0; i < top2; i++ ) {
        l = l2[i]; c = c2[i];
        vizdist2[l][c] = travtop;
        dist2[l][c] = 0;
        ql[ultim] = l; qc[ultim] = c;
        ultim = (ultim+1)&(QMAX-1);
    }
    traversare(tabla, dist2, vizdist2);

    int insule, insule1, insule2;
    insule = insule1 = insule2 = 0;

    for ( l = 1; l <= n; l++ ) {
        for ( c = 1; c <= m; c++ ) {
            if ( tabla[l][c] != 'x' && viz[l][c] == 0 )
                compid[l][c] = 0;
        }
    }

    for ( l = 1; l <= n; l++ ) {
        for ( c = 1; c <= m; c++ ) {
            d1 = (vizdist1[l][c] == travtop) ? dist1[l][c] : 1000;
            d2 = (vizdist2[l][c] == travtop) ? dist2[l][c] : 1000;
            if ( d1 < 1000 && d2 == 1000 )
                enclava1++;
            else if ( d1 == 1000 && d2 < 1000 )
                enclava2++;
            else if ( d1 < d2 ) {
                vor1++;
                av1 += (d2-d1);
            } else if ( d2 < d1 ) {
                vor2++;
                av2 += (d1-d2);
            } else if ( d1 == d2 && d1 < 1000 ) {
                if ( juc == 1 )
                    vor1++;
                else
                    vor2++;
            }

            if ( tabla[l][c] != 'x' && viz[l][c] == 0 && compid[l][c] == 0 ) {
                insule++;
                comparie[insule] = compatins1[insule] = compatins2[insule] = 0;

                p = u = 0;
                int coadal[2500], coadac[2500];
                coadal[u] = l; coadac[u++] = c;
                compid[l][c] = insule;

                while ( p < u ) {
                    tmpl = coadal[p]; tmpc = coadac[p++];
                    comparie[insule]++;

                    if ( vizdist1[tmpl][tmpc] == travtop && dist1[tmpl][tmpc] == 1 )
                        compatins1[insule] = 1;
                    if ( vizdist2[tmpl][tmpc] == travtop && dist2[tmpl][tmpc] == 1 )
                        compatins2[insule] = 1;

                    for ( d = 0; d < 4; d++ ) {
                        nl = tmpl + dl[d]; nc = tmpc + dc[d];
                        if ( tabla[nl][nc] == tabla[l][c] && viz[nl][nc] == 0 && compid[nl][nc] == 0) {
                            compid[nl][nc] = insule;
                            coadal[u] = nl; coadac[u++] = nc;
                        }
                    }
                }
                if ( compatins1[insule] && !compatins2[insule] )
                    insule1 += comparie[insule];
                else if ( !compatins1[insule] && compatins2[insule] )
                    insule2 += comparie[insule];
            }
        }
    }

    double total = (double)(ar1+ar2)/(n*m);
    if ( total < 0.30 ) {
        sc1 = 1000 * (ar1+enclava1+insule1) + 600 * vor1 + 20 * av1;
        sc2 = 1000 * (ar2+enclava2+insule2) + 600 * vor2 + 20 * av2;
    } else if ( total < 0.75 ) {
        sc1 = 1000 * (ar1+enclava1+insule1) + 300 * vor1 + 15 * av1;
        sc2 = 1000 * (ar2+enclava2+insule2) + 300 * vor2 + 15 * av2;
    } else {
        sc1 = 1000 * (ar1+enclava1+insule1) + 50 * vor1 + 5 * av1;
        sc2 = 1000 * (ar2+enclava2+insule2) + 50 * vor2 + 5 * av2;
    }

    return sc1-sc2;
}

// negamax cu killer moves, iterative deepening, tabele de transpozitie (zobrist hashing)
// si un pic de ordonarea mutarilor
int negamax(char tabla[NMAX+2][NMAX+2], int nivel, int a, int b, 
                char juc, char charJ, char charS) {
    if ( timp_verif == 0 )
        return a;

    if ( (++noduri & 15) == 0 ) {
        noduri = 0;
        if ( getTime() - last_timp > 985000 ) {
            timp_verif = 0;
            return a;
        }
    }

    int alpha = a,
        adanramas = nivel_max - nivel,
        ttidx = zobcur & (TSIZE-1);

    if ( tt[ttidx] == zobcur && ttadancime[ttidx] >= adanramas ) {
        if ( ttid[ttidx] == 0 )
            return ttscor[ttidx];
        if ( ttid[ttidx] == 1 && ttscor[ttidx] <= a )
            return a;
        if ( ttid[ttidx] == 2 && ttscor[ttidx] >= b )
            return b;
    }

    char ttbest = (tt[ttidx] == zobcur) ? ttmutare[ttidx] : 'x';

    int ret_eval, cv, tmpl, tmpc, l, c;
    char viz[NMAX+2][NMAX+2];
    int culori[256] = {0};

    top1 = top2 = 0;
    memset(viz, 0, sizeof(viz)); 

    if ( nivel == nivel_max ) {
        top++;
        flood_viz(n, 1, charJ, 1, tabla, viz, (juc == 1) ? culori : NULL);
        top++;
        flood_viz(1, m, charS, 2, tabla, viz, (juc == 2) ? culori : NULL);

        ret_eval = eval(tabla, viz, juc);
        return (juc == 1) ? ret_eval : -ret_eval;
    } else {
        top++;
        if ( juc == 1 )
            flood_viz(n, 1, charJ, 1, tabla, viz, culori);
        else
            flood_viz(1, m, charS, 2, tabla, viz, culori);
    }

    int are = 0, j = 0;
    while ( j < 5 ) {
        if ( caractere[j] != charS && caractere[j] != charJ && culori[(int)caractere[j]] > 0 )
            are = 1;
        j++;
    }

    int i = 0, ans; unsigned int tmp;
    char singura; unsigned char old, neww;

    int scorbun = -INF;
    char mutarebuna = 'x', mutare;

    if ( are == 0 ) {
        singura = 'x';
        i = 0;
        while ( i < 5 ) {
            if ( caractere[i] != charS && caractere[i] != charJ ) 
                singura = caractere[i];
            i++;
        }

        cv = modiftop;
        top++;
        zobcur ^= zobjuc;
        if ( juc == 1 )
            flood_update(n, 1, singura, 1, tabla, viz);
        else
            flood_update(1, m, singura, 2, tabla, viz);

        if ( juc == 1 )
            ans = -negamax(tabla, nivel+1, -b, -a, 2, singura, charS);
        else
            ans = -negamax(tabla, nivel+1, -b, -a, 1, charJ, singura);
        
        while ( modiftop > cv ) {
            tmp = modif[--modiftop];
            tmpl = (tmp>>16)&0xFF; tmpc = (tmp>>8)&0xFF;
            old = (unsigned char)(tmp&0xFF); neww = (unsigned char)tabla[tmpl][tmpc];
            zobcur ^= zob[tmpl][tmpc][neww] ^ zob[tmpl][tmpc][old];
            tabla[tmpl][tmpc] = old;
        }
        zobcur ^= zobjuc;

        if ( timp_verif == 0 )
            return a;

        scorbun = ans;

        if ( ans > a )
            a = ans;

        return a;

    } else {

        char killer = kill[nivel];
        char ord[5], aux; memcpy(ord, caractere, 5);

        for ( l = 0; l < 4; l++ ) {
            for ( c = l+1; c < 5; c++ ) {
                tmpl = culori[(int)ord[l]]; tmpc = culori[(int)ord[c]];
                if ( ord[l] == ttbest )
                    tmpl += 10000;
                else if ( ord[l] == killer )
                    tmpl += 5000;
                if ( ord[c] == ttbest )
                    tmpc += 10000;
                else if ( ord[c] == killer )
                    tmpc += 5000;
                if ( tmpl < tmpc ) {
                    aux = ord[l];
                    ord[l] = ord[c];
                    ord[c] = aux;
                }
            }
        }

        for ( i = 0; i < 5; i++ ) {
            mutare = ord[i];
            if ( mutare != charS && mutare != charJ && culori[(int)mutare] != 0 ) {
                cv = modiftop;
                top++;
                zobcur ^= zobjuc;
                if ( juc == 1 )
                    flood_update(n, 1, mutare, 1, tabla, viz);
                else
                    flood_update(1, m, mutare, 2, tabla, viz);
                
                if ( juc == 1 )
                    ans = -negamax(tabla, nivel+1, -b, -a, 2, mutare, charS);
                else
                    ans = -negamax(tabla, nivel+1, -b, -a, 1, charJ, mutare);

                while ( modiftop > cv ) {
                    tmp = modif[--modiftop];
                    tmpl = (tmp>>16)&0xFF; tmpc = (tmp>>8)&0xFF;
                    old = (unsigned char)(tmp&0xFF); neww = (unsigned char)tabla[tmpl][tmpc];
                    zobcur ^= zob[tmpl][tmpc][neww] ^ zob[tmpl][tmpc][old];
                    tabla[tmpl][tmpc] = old;
                }
                zobcur ^= zobjuc;

                if ( timp_verif == 0 )
                    return a;

                if ( ans > scorbun ) {
                    scorbun = ans;
                    mutarebuna = mutare;
                }
                if ( ans > a ) {
                    a = ans;
                    if ( a >= b ) {
                        kill[nivel] = mutare;
                        break;
                    }
                }
            }
        }
    }

    if ( timp_verif ) {
        tt[ttidx] = zobcur;
        ttadancime[ttidx] = adanramas;
        ttscor[ttidx] = scorbun;
        ttmutare[ttidx] = mutarebuna;
        if ( scorbun <= alpha )
            ttid[ttidx] = 1;
        else if ( scorbun >= b )
            ttid[ttidx] = 2;
        else
            ttid[ttidx] = 0;
    }
    
    return a;
}

//--------------------------------------//

int main(void) {

    last_timp = getTime();

    setvbuf(stdout, NULL, _IOFBF, 4096);

    zobinit();
    char tabla[NMAX+2][NMAX+2], viz[NMAX+2][NMAX+2];

    char citire, poz, charJ, charS;
    int l, c, curM, i;
    l = c = n = m = curM = 1;

    poz = getchar();
    citire = getchar();

    // citire
    while ( (citire = getchar()) != EOF ) {
        if ( citire == '\n' ) {
            if ( curM > 1 ) {
                m = curM - 1;
                curM = 1;
                n++;
            }
        } else {
            tabla[n][curM++] = citire;
        }
    }

    if ( curM == 1 ) 
        n--;

    // bordare
    for ( l = 0; l <= n+1; l++ ) {
        tabla[l][0] = tabla[l][m+1] = 'x';
        viz[l][0] = viz[l][m+1] = MARGINE;
    }

    for ( c = 0; c <= m+1; c++ ) {
        tabla[0][c] = tabla[n+1][c] = 'x';
        viz[0][c] = viz[n+1][c] = MARGINE;
    }

    charJ = tabla[n][1];
    charS = tabla[1][m];

    //--------------------------------------//

    int adancime, cv, tmpl, tmpc;
    unsigned char old, neww;
    char char_bun = 'x';

    i = 0;
    while ( i < 5 && (caractere[i] == charJ || caractere[i] == charS) )
        i++;
    // ma asigur ca va face o miscare
    char_bun = caractere[i]; 

    timp_verif = 1;

    for ( i = 0; i < 100; i++ )
        kill[i] = 'x';

    memset(temp, 0, sizeof(temp));
    modiftop = 0;

    zobcur = 0;
    for ( l = 1; l <= n; l++ ) {
        for ( c = 1; c <= m; c++ ) {
            if ( tabla[l][c] != 'x' ) {
                zobcur ^= zob[l][c][(unsigned char)tabla[l][c]];
            }
        }
    }

    //--------------------------------------//

    int max, ans;
    unsigned int tmp;
    char tmp_char_bun;
    adancime = 1;

    // finally am unificat if-urile imense :o

    if ( poz == 'S' )
        zobcur ^= zobjuc;

    // nu cred ca ajunge la 50 (v8?). asa ca am pus la 75 :)
    while ( adancime <= 75 && timp_verif == 1 ) {
        max = -INF;
        tmp_char_bun = 'x';
        nivel_max = adancime;
        noduri = 0;

        char viz2[NMAX+2][NMAX+2];
        i = 0;

        char ord[5], aux; memcpy(ord, caractere, 5);
        if ( char_bun != 'x' ) {
            i = 0;
            while ( i < 5 && ord[i] != char_bun ) 
                i++;
            aux = ord[0];
            ord[0] = ord[i];
            ord[i] = aux;
        }

        i = 0;
        while ( i < 5 && timp_verif == 1 ) {
            if ( ord[i] != charJ && ord[i] != charS ) {
                    
                memset(viz2, 0, sizeof(viz2));
                top++;
                flood_viz(n, 1, charJ, 1, tabla, viz2, NULL);
                top++;
                flood_viz(1, m, charS, 2, tabla, viz2, NULL);

                cv = modiftop;

                top++;
                zobcur ^= zobjuc;
                flood_update((poz=='J')?n:1, (poz=='J')?1:m, ord[i], (poz=='J')?1:2, tabla, viz2);

                ans = -negamax(tabla, 0, -INF, INF, (poz=='J')?2:1, (poz=='J')?ord[i]:charJ, (poz=='J')?charS:ord[i]);

                while ( modiftop > cv ) {
                    tmp = modif[--modiftop];
                    tmpl = (tmp>>16)&0xFF; tmpc = (tmp>>8)&0xFF;
                    old = (unsigned char)(tmp&0xFF); neww = (unsigned char)tabla[tmpl][tmpc];
                    zobcur ^= zob[tmpl][tmpc][neww] ^ zob[tmpl][tmpc][old];
                    tabla[tmpl][tmpc] = old;
                }
                zobcur ^= zobjuc;

                if ( ans > max && timp_verif ) {
                    max = ans;
                    tmp_char_bun = ord[i];
                }
            }
            i++;
        }

        if ( timp_verif == 1 && tmp_char_bun != 'x' )
            char_bun = tmp_char_bun;
        adancime++;
    }

    memset(viz, 0, sizeof(viz));
    top++;
    flood_viz(n, 1, charJ, 1, tabla, viz, NULL);
    top++;
    flood_viz(1, m, charS, 2, tabla, viz, NULL);
    top++;
    flood_update((poz=='J')?n:1, (poz=='J')?1:m, char_bun, (poz=='J')?1:2, tabla, viz);

    if ( poz == 'J' )
        poz = 'S';
    else
        poz = 'J';

    putc(poz, stdout); putc('\n', stdout);

    for ( l = 1; l <= n; l++ ) {
        for ( c = 1; c <= m; c++ ) 
            putc(tabla[l][c], stdout);
        putc('\n', stdout);
    }

    return 0;
    
}
// salutari
