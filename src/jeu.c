/*
    Canvas pour algorithmes de jeux à 2 joueurs

    joueur 0 : humain
    joueur 1 : ordinateur

*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>
#include <unistd.h>
#include <limits.h>

// Paramètres du jeu
#define LARGEUR_MAX 7       // nb max de fils pour un noeud (= nb max de coups possibles)

#define TEMPS 1     // temps de calcul pour un coup avec MCTS (en secondes)

#define WIDTH 7
#define HEIGHT 6

// macros
#define AUTRE_JOUEUR(i) (1-(i))
#define min(a, b)       ((a) < (b) ? (a) : (b))
#define max(a, b)       ((a) < (b) ? (b) : (a))

// Critères de fin de partie
typedef enum {
    NON, MATCHNUL, ORDI_GAGNE, HUMAIN_GAGNE
} FinDePartie;

// Definition du type Etat (état/position du jeu)
typedef struct EtatSt {

    int joueur; // à qui de jouer ? 
    /* par exemple, pour morpion: */
    char plateau[HEIGHT][WIDTH];

} Etat;

// Definition du type Coup
typedef struct {
    /* par exemple, pour morpion: */
    int ligne;
    int colonne;

} Coup;

// Copier un état
Etat *copieEtat(Etat *src) {
    Etat *etat = (Etat *) malloc(sizeof(Etat));

    etat->joueur = src->joueur;
    /* par exemple : */
    int i, j;
    for (i = 0; i < HEIGHT; i++)
        for (j = 0; j < WIDTH; j++)
            etat->plateau[i][j] = src->plateau[i][j];


    return etat;
}

// Etat initial
Etat *etat_initial(void) {
    Etat *etat = (Etat *) malloc(sizeof(Etat));
    int i, j;
    for (i = 0; i < HEIGHT; i++)
        for (j = 0; j < WIDTH; j++)
            etat->plateau[i][j] = ' ';

    return etat;
}


void afficheJeu(Etat *etat) {
	fflush(stdout);
    int i, j;
    printf("|");
    for (j = 0; j < WIDTH; j++)
        printf(" %d |", j);
    printf("\n");
    printf("-----------------------------");
    printf("\n");

    for (i = 0; i < HEIGHT; i++) {
        printf("|");
        for (j = 0; j < WIDTH; j++)
            printf(" %c |", etat->plateau[i][j]);
        printf("\n");
        printf("-----------------------------");
        printf("\n");
    }
}


// Nouveau coup
Coup *nouveauCoup(int i, int j) {
    Coup *coup = (Coup *) malloc(sizeof(Coup));

    coup->ligne = i;
    coup->colonne = j;

    return coup;
}

// Demander à l'humain quel coup jouer
Coup *demanderCoup(Etat *etat) {
    int i, j;
    printf(" quelle colonne ? ");
    scanf("%d", &j);
    for (i = 0; i < HEIGHT; ++i) {
        if (etat->plateau[i][j] != ' '){
            break;
        }
    }

    return nouveauCoup(i-1, j);
}

// Modifier l'état en jouant un coup
// retourne 0 si le coup n'est pas possible
int jouerCoup(Etat *etat, Coup *coup) {
    if (etat->plateau[coup->ligne][coup->colonne] != ' ')
        return 0;
    else {
        etat->plateau[coup->ligne][coup->colonne] = etat->joueur ? 'O' : 'X';
        etat->joueur = AUTRE_JOUEUR(etat->joueur);
        return 1;
    }
}

// Retourne une liste de coups possibles à partir d'un etat
// (tableau de pointeurs de coups se terminant par NULL)
Coup **coups_possibles(Etat *etat) {

    Coup **coups = (Coup **) malloc((1 + LARGEUR_MAX) * sizeof(Coup *));

    int k = 0;
    int i, j;
    for (j = 0; j < WIDTH; j++) {
        for (i = 0; i < HEIGHT; ++i) {
            if (etat->plateau[i][j] != ' '){
                break;
            }
        }
        if (i > 0){
            coups[k] = nouveauCoup(i-1, j);
            k++;
        }
    }

    coups[k] = NULL;

    return coups;
}


// Definition du type Noeud
typedef struct NoeudSt {

    int joueur; // joueur qui a joué pour arriver ici
    Coup *coup;   // coup joué par ce joueur pour arriver ici

    Etat *etat; // etat du jeu

    struct NoeudSt *parent;
    struct NoeudSt *enfants[LARGEUR_MAX]; // liste d'enfants : chaque enfant correspond à un coup possible
    int nb_enfants; // nb d'enfants présents dans la liste

    // POUR MCTS:
    float nb_victoires;
    int nb_simus;

} Noeud;


// Créer un nouveau noeud en jouant un coup à partir d'un parent
// utiliser nouveauNoeud(NULL, NULL) pour créer la racine
Noeud *nouveauNoeud(Noeud *parent, Coup *coup) {
    Noeud *noeud = (Noeud *) malloc(sizeof(Noeud));

    if (parent != NULL && coup != NULL) {
        noeud->etat = copieEtat(parent->etat);
        jouerCoup(noeud->etat, coup);
        noeud->coup = coup;
        noeud->joueur = AUTRE_JOUEUR(parent->joueur);
    } else {
        noeud->etat = NULL;
        noeud->coup = NULL;
        noeud->joueur = 0;
    }
    noeud->parent = parent;
    noeud->nb_enfants = 0;

    // POUR MCTS:
    noeud->nb_victoires = 0;
    noeud->nb_simus = 0;


    return noeud;
}

// Ajouter un enfant à un parent en jouant un coup
// retourne le pointeur sur l'enfant ajouté
Noeud *ajouterEnfant(Noeud *parent, Coup *coup) {
    Noeud *enfant = nouveauNoeud(parent, coup);
    parent->enfants[parent->nb_enfants] = enfant;
    parent->nb_enfants++;
    return enfant;
}

void freeNoeud(Noeud *noeud) {
    if (noeud->etat != NULL)
        free(noeud->etat);

    while (noeud->nb_enfants > 0) {
        freeNoeud(noeud->enfants[noeud->nb_enfants - 1]);
        noeud->nb_enfants--;
    }
    if (noeud->coup != NULL)
        free(noeud->coup);

    free(noeud);
}


// Test si l'état est un état terminal
// et retourne NON, MATCHNUL, ORDI_GAGNE ou HUMAIN_GAGNE
FinDePartie testFin(Etat *etat) {
    int n = 0;
    int k = 0;
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            if (etat->plateau[i][j] != ' '){
                //compte le nombre de coup
                n++;
                //test des colones
                k = 0;
                while(k < 4 && i + k < HEIGHT && etat->plateau[i + k][j] == etat->plateau[i][j]){
                    k++;
                }
                if(k == 4){
                    return (etat->plateau[i][j] == 'O')? ORDI_GAGNE : HUMAIN_GAGNE;
                }
                //test des lignes
                k = 0;
                while(k < 4 && j + k < WIDTH && etat->plateau[i][j + k] == etat->plateau[i][j]){
                    k++;
                }
                if(k == 4){
                    return (etat->plateau[i][j] == 'O')? ORDI_GAGNE : HUMAIN_GAGNE;
                }
                //test des diagonales : \.
                k = 0;
                while(k < 4 && i + k < HEIGHT && j + k < HEIGHT && etat->plateau[i + k][j + k] == etat->plateau[i][j]){
                    k++;
                }
                if(k == 4){
                    return (etat->plateau[i][j] == 'O')? ORDI_GAGNE : HUMAIN_GAGNE;
                }
                //test des diagonales : /
                k = 0;
                while(k < 4 && i - k >= 0 && j + k < WIDTH && etat->plateau[i - k][j + k] == etat->plateau[i][j]){
                    k++;
                }
                if(k == 4){
                    return (etat->plateau[i][j] == 'O')? ORDI_GAGNE : HUMAIN_GAGNE;
                }
            }
        }
    }
    return (n >= WIDTH * HEIGHT)? MATCHNUL: NON;
}


// Calcule et joue un coup de l'ordinateur avec MCTS-UCT
// en tempsmax secondes
void ordijoue_mcts(Etat *etat, int tempsmax) {

    clock_t tic, toc;
    tic = clock();
    int temps;
    srand(time(NULL));

    Coup **coups;
    Coup *meilleur_coup;

    // Créer l'arbre de recherche
    Noeud *racine = nouveauNoeud(NULL, NULL);
    racine->etat = copieEtat(etat);

    // créer les premiers noeuds:
    coups = coups_possibles(racine->etat);


    int k = 0;
    Noeud *enfant;
    while (coups[k] != NULL) {
        enfant = ajouterEnfant(racine, coups[k]);
        k++;
    }


    // meilleur_coup = coups[rand() % k]; // choix aléatoire

    //TODO :
    //    - supprimer la sélection aléatoire du meilleur coup ci-dessus
    //    - implémenter l'algorithme MCTS-UCT pour déterminer le meilleur coup ci-dessous

    int iter = 0;
	

    do {
        // Selectioner le noeud avec la plus grande B-Valeur recurcivement jusqu'a un fils sans B-valeur
        float maxBvaleur = 0;
        Noeud *noeudMaxBvaleur;
        bool trouve = false;
        Noeud *noeudCourant = racine;
        Noeud *enfantsTrouve[LARGEUR_MAX];
        int nbEnfantsTrouve = 0;

		int a = 1;
		
        while(!trouve){
			a++;
			maxBvaleur = INT_MIN;
            if (testFin(noeudCourant->etat) == NON) {
                for (int i = 0; i < noeudCourant->nb_enfants; ++i) {
                    Noeud *noeudEnfantCourant = noeudCourant->enfants[i];
                    if (noeudEnfantCourant->nb_simus == 0) {
                        trouve = true;
                        enfantsTrouve[nbEnfantsTrouve] = noeudEnfantCourant;
                        nbEnfantsTrouve++;
                    } else {
                        float muI = (float) noeudEnfantCourant->nb_victoires / (float) noeudEnfantCourant->nb_simus;
                        if (noeudEnfantCourant->joueur == 0){
                            muI = -muI;
                        }
                        float Bvaleur =
                                muI + sqrt(2) * sqrt(log(noeudCourant->nb_simus) / noeudEnfantCourant->nb_simus);
						if (Bvaleur > maxBvaleur) {
							maxBvaleur = Bvaleur;
                            noeudMaxBvaleur = noeudEnfantCourant;
                        }
                    }
                }
                if (!trouve) {
                    noeudCourant = noeudMaxBvaleur;
                    if (noeudCourant->nb_enfants == 0) {
                        coups = coups_possibles(noeudCourant->etat);
                        k = 0;
                        while (coups[k] != NULL) {
                            ajouterEnfant(noeudCourant, coups[k]);
                            k++;
                        }
                    }
                }
            }else{
                enfantsTrouve[nbEnfantsTrouve] = noeudCourant;
                nbEnfantsTrouve++;
                trouve = true;
            }
        }

        // avec les fils sans B-Valeur => en choisir un aleatoirement
        Noeud *noeudChoisi = enfantsTrouve[rand() % nbEnfantsTrouve];

        //simuler une partie aléatoire
        /*Etat *etatDepart = copieEtat(noeudChoisi->etat);

        while(testFin(etatDepart) == NON){
            Coup **coupsDePartie = coups_possibles(etatDepart);
            int nbCoups = 0;
            while (coupsDePartie[nbCoups] != NULL) {
                nbCoups++;
            }
            Coup* coupChoisi = coupsDePartie[rand() % nbCoups];
            jouerCoup(etatDepart, coupChoisi);
        }
		*/
		
		//simuler une partie aléatoire
        Etat *etatDepart = copieEtat(noeudChoisi->etat);
        Etat *etatFuture = copieEtat(etatDepart);
        bool etatFuturEstFinal = false;

        while(testFin(etatDepart) == NON){
			etatFuturEstFinal = false;
            Coup **coupsDePartie = coups_possibles(etatDepart);
            int nbCoups = 0;
            while (coupsDePartie[nbCoups] != NULL) {
                nbCoups++;
            }
            for (int i = 0; i < nbCoups; i++) {
                etatFuture = copieEtat(etatDepart);
                jouerCoup(etatFuture, coupsDePartie[i]);
                if (testFin(etatFuture) == ORDI_GAGNE){
                    etatFuturEstFinal = true;
                    jouerCoup(etatDepart, coupsDePartie[i]);
                    break;
                }
            }
            if (!etatFuturEstFinal){
                Coup* coupChoisi = coupsDePartie[rand() % nbCoups];
                jouerCoup(etatDepart, coupChoisi);
            }
        }

        // remonter la valeur sur tout les neuds parcouru jusqu'a la racine
        FinDePartie resultat = testFin(etatDepart);
        noeudCourant = noeudChoisi;
        while(noeudCourant != NULL){
            if (resultat == ORDI_GAGNE){
                noeudCourant->nb_victoires += 1;
            }
            if (resultat == MATCHNUL){
                noeudCourant->nb_victoires += 0.5f;
            }
            noeudCourant->nb_simus++;
            noeudCourant = noeudCourant->parent;
        }

        free(etatDepart);
	

        toc = clock();
        temps = (int)( ((double) (toc - tic)) / CLOCKS_PER_SEC );
        iter ++;
    } while (temps < tempsmax);

    //fin de l'algorithme
    int maxN = 0;
    for (int j = 0; j < racine->nb_enfants; j++) {
        if (racine->enfants[j]->nb_victoires > maxN){
            meilleur_coup = racine->enfants[j]->coup;
            maxN = racine->enfants[j]->nb_simus;
        }
    }

    // Jouer le meilleur premier coup
    jouerCoup(etat, meilleur_coup);
	
	printf("\nnombre de simulations : %d\n", iter);
	printf("estimation de la probabilité de victoire pour l'ordinateur %f \n", racine->nb_victoires/(float)racine->nb_simus);

    // Penser à libérer la mémoire :
    freeNoeud(racine);
    free(coups);
}

int main(void) {

    Coup *coup;
    FinDePartie fin;

    // initialisation
    Etat *etat = etat_initial();

    // Choisir qui commence :
    printf("Qui commence (0 : humain, 1 : ordinateur) ? ");
    scanf("%d", &(etat->joueur));

    // boucle de jeu
    do {
        printf("\n");
        afficheJeu(etat);
        Coup** coup2 = coups_possibles(etat);
        /*printf("Coup possible(s):\n");
        for (int i = 0; i < 7; ++i) {
            printf("{%d | %d}",coup2[i]->colonne, coup2[i]->ligne);
        }*/

        if (etat->joueur == 0) {
            // tour de l'humain

            do {
                coup = demanderCoup(etat);
            } while (!jouerCoup(etat, coup));

        } else {
            // tour de l'Ordinateur

            ordijoue_mcts(etat, TEMPS);

        }

        fin = testFin(etat);
    } while (fin == NON);

    printf("\n");
    afficheJeu(etat);

    if (fin == ORDI_GAGNE)
        printf("** L'ordinateur a gagné **\n");
    else if (fin == MATCHNUL)
        printf(" Match nul !  \n");
    else
        printf("** BRAVO, l'ordinateur a perdu  **\n");
    return 0;
}
