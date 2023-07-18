#include <stdio.h>
#include <iostream>
#include <math.h>
#include <windows.h>
#include <stdlib.h>
#include <cstdlib>
#include <time.h>
#include <string> 

#pragma warning (disable : 4996)//para crear ek archivo

#include <allegro5/allegro.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_native_dialog.h>
#include <allegro5/allegro_primitives.h> 
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>

using namespace std;

//--- Variables Globales ---
#define FPS 60.0 //Variable para la velocidad del timer.
ALLEGRO_FONT* fuente;

//Buffers para la conversion de numero a string.
char Letras[3000];
char Tiempo[300];
char Pistas[300];
char Pistas2[300];
char actual[1];//Char utilizado como auxiliar para la funcion dibujar.

typedef struct grupo//Este tipo de datos seran los grupos de letras que formaran las cascadas de caracteres.
{
	char carac[6]; //Array de chars que se utilizaran para realizar el degradado de la cascada.
	int X;  //coordenada en X para el caracter de mas abajo de una hilada
	int Y;  //coordenada en Y para el caracter de mas abajo de una hilada
	int contador;  //Sirve para contar los caracteres que faltan de salir de la ventana cuando la hilera llega al final
	int limite; //Valor el cual delimitara el limite maximo en la coordenada y de la cascada.
	int colorActual;//Valor que se utilizara para definir el tono de verde de la cascada.
	bool draw;//Booleano que denotara dos estados de la cascada, true para dibujar y false para borrar.
};

typedef struct Tvalor //Tipo de dato para manejar una lista enlazada, donde solo se tiene un valor numerico como parametro.
{
	int valor; //Valor numerico, este representa el numero de pista ocupado.
	Tvalor* Siguiente; //Declaracion recursiva donde se genera un puntero del mismo tipo del struct creado. 
}*PtrTvalor;

//LAS SIGUIENTES DOS FUNCIONES SE USAN PARA GENERAR NUMEROS RANDOM "VERDADEROS"
//EN PERIODOS DE TIEMPO MUY CORTOS
long g_seed = 1;//
inline int fastrand()//Subrutina la cual permite generar valores aleatorios en intervalos extremadamente cortos de tiempo.
{
	g_seed = (214013 * g_seed + 2531011);
	return (g_seed >> 16) & 0x7FFF;
}
char GenerarRandom()//Subrutina que devuelve un char random dentro de un rango, el cual, por temas de la fuente utilizada solo pueden ser el abecedario en mayuscula y minuscula y los numero.
{
	char c;
	bool comprobar = true;
	while (comprobar)
	{
		int num = 48 + fastrand() % 74; //Genera un numero que puede caer dentro del rango buscado.
		if (((num >= 48 && num <= 57) || (num >= 65 && num <= 90) || (num >= 97 && num <= 122)) && num != 83 && num != 88 && num != 113)//Se verifica que el valor este en rango.
		{
			c = (char)num;//Si esta en rango se convierte el entero a un char y se termina el while.
			comprobar = false;
		}
	}
	return c;//Se retorna el caracter.
}

void inicializarstack(grupo& cascada)//Subrutina que recibe un que representa una cascada de caracteres, e inicializa el stack de caracteres de la misma en vacio.
{
	for (int i = 0; i < 6; i++)
	{
		cascada.carac[i] = ' ';
	}
}

void InicializarInventario(PtrTvalor& Lista)//Inicializa una lista enlazada, tomada de ejemplos antiguos.
{
	Lista = NULL;
}

void DestruirInventario(PtrTvalor& Lista)//Destruye una lista enlazada, tomada de ejemplos antiguos.
{
	PtrTvalor Aux;
	Aux = Lista;
	while (Aux != NULL)
	{
		Lista = Lista->Siguiente;
		delete(Aux);
		Aux = Lista;
	}
}

PtrTvalor CrearArticulo(int val) //Crea un elemto de tipo Tvalor con un valor dado y lo retorna, modificada de ejemplos antiguos.
{
	PtrTvalor valor = new(Tvalor);//Crea el elemento
	//Lo inicializa.
	valor->valor = val;

	valor->Siguiente = NULL;
	return valor;//Lo retorna.
}

void AgregarInicio(PtrTvalor& Lista, PtrTvalor& Nuevo)//Agrega un elemento al inicio de una lista enlazada, tomada de ejemplos antiguos.
{
	Nuevo->Siguiente = Lista;
	Lista = Nuevo;
}

bool estaEnLista(PtrTvalor& Lista, int cual)//Recorre una lista enlazada, y si algun nodo tiene su parametro val equivalente a un entero pasada retorna true, sino retorna false, tomada de ejemplos antiguos.
{
	bool comp = false;
	PtrTvalor Aux = Lista;
	while (comp != true && Aux != NULL)
	{
		if (Aux->valor == cual)
		{
			//Si encuentra coincidencia.
			return true;
			comp = true;
		}
		else
		{
			//Si no encuentra coincidencia y el while no se ha terminado.
			Aux = Aux->Siguiente;
		}
	}
	//Si no se encuentra coincidencia.
	return false;
}

bool Borrar(PtrTvalor& Lista, int cual)//Borra un elemento de una lista enlazada, buscando la coincidencia del parametro val de los nodos con un entero pasado, tomada de ejemplos antiguos.
{
	bool comp = false;
	PtrTvalor Aux = Lista;
	PtrTvalor Kill = NULL;
	while (comp == false && Aux->Siguiente != NULL && Aux != NULL)
	{
		if (Aux->Siguiente->valor == cual)
		{
			//En caso de quela coincidencia ocurra y no sea el primer nodo. 
			comp = true;
			Kill = Aux->Siguiente;
			Aux->Siguiente = Aux->Siguiente->Siguiente;
			delete(Kill);
		}
		else if (Aux->valor == cual) {
			//En caso de que la coincidencia sea del primer nodo.
			comp = true;
			Kill = Aux;
			Lista = Aux->Siguiente;
			delete(Kill);
		}
		Aux = Aux->Siguiente;//En caso de que el while no haya terminado y aun no se encuentre coincidencia.
	}
	return comp;//Retorna si se logro la eliminacion.
}

/*
CADA LINEA DE CARACTERES QUE VA CAYENDO SE GUARDA EN UN STACK, QUE ES UN ARREGLO DE CHAR
COMO SE VAN GENERANDO CARACTERES NUEVOS ENTONCES LOS CARACTERES EN EL STACK SE VAN DESPLAZANDO
A MODO DE UNA PILA FIFO, PERO COMO ES UN ARREGLO ENTONCES SE IMPLEMENTA MAS FACIL DE LA SIGUIENTE MANERA
*/
void desplazar(char stack[6]) {
	stack[5] = stack[4];
	stack[4] = stack[3];
	stack[3] = stack[2];
	stack[2] = stack[1];
	stack[1] = stack[0];
}

/*
DIBUJAR LO QUE HACE ES EN BASE A UN STACK DEFINIDO Y UN CONJUNTO DE COORDENADAS IMPRIME EN EL DISPLAY
LA SECUENCIA DE CARACTERES ASCII, Y A SU VEZ LLAMA A LA FUNCION DESPLAZAR PARA SACAR DEL STACK EL CARACTER MAS VIEJO Y
AÃ‘ADIR AL INICIO EL MAS NUEVO
*/

/*Subrutina dibujar, la cual al obtener una lista con el nuevo caracter, el esta de una grupo de letras con las mas actuales
, la coordenada "x" y "y" y un entero para el color dibujara en pantalla los valores correspondientes, ademas de agreagra al
stack del grupo de letras el nuevo caracter en la posicion 0, despues de haber realizado un desplazamiento de todos los valores*/
void dibujar(char caracter[1], char stack[6], int x, int y, int color) {
	/*Se coloca en actual el valor a imprimir, donde se empieza imprimiendo el valor del stack en 0, hasta el 5, empezando por
	la posicion siperior al nuevo valor y subiendo en cada cambio, ademas de generar el degradado del color.*/
	actual[0] = stack[0];
	al_draw_text(fuente, al_map_rgb(170, color, 170), 30 + (x * 26), y - 20, ALLEGRO_ALIGN_CENTRE, actual);
	actual[0] = stack[1];
	al_draw_text(fuente, al_map_rgb(150, color, 150), 30 + (x * 26), y - 40, ALLEGRO_ALIGN_CENTRE, actual);
	actual[0] = stack[2];
	al_draw_text(fuente, al_map_rgb(125, color, 125), 30 + (x * 26), y - 60, ALLEGRO_ALIGN_CENTRE, actual);
	actual[0] = stack[3];
	al_draw_text(fuente, al_map_rgb(100, color, 100), 30 + (x * 26), y - 80, ALLEGRO_ALIGN_CENTRE, actual);
	actual[0] = stack[4];
	al_draw_text(fuente, al_map_rgb(50, color, 50), 30 + (x * 26), y - 100, ALLEGRO_ALIGN_CENTRE, actual);
	actual[0] = stack[5];
	al_draw_text(fuente, al_map_rgb(0, color, 0), 30 + (x * 26), y - 120, ALLEGRO_ALIGN_CENTRE, actual);
	//Despues de imprimir el degradado, se imprime el mievo caracter con color blanco en la debajo de todo lo anterior.
	al_draw_text(fuente, al_map_rgb(255, 255, 255), 30 + (x * 26), y, ALLEGRO_ALIGN_CENTRE, caracter);

	al_flip_display();

	desplazar(stack);//Se desplaza el stack perdiendo el valor en el 5.
	stack[0] = caracter[0];//Se coloca el nuevo caracter en la posicion 0, para la proxima llamada.
}
/*Esta funcion recibe un valor x y un valor y, donde realiza los cambios pertinentes para ajusta la posicion
e imprime un recangulo relleno negro en la posicion indicada.*/
void dibujar2(int x, int y) {
	x = 30 + (x * 26);
	x -= 13;
	al_draw_filled_rectangle(x, y, (x + 26), (y + 24), al_map_rgb(0, 0, 0));
	al_flip_display();
}

int color() {//Genera un valor aletorio de 0 a 3, dependiendo del resultado el switch devolvera un numero que sera el color para los grupos de nuemeros.
	int numC = fastrand() % 4;
	switch (numC) {
	case 0:
		return 180;
		break;
	case 1:
		return 200;
		break;
	case 2:
		return 220;
		break;
	case 3:
		return 255;
		break;
	}
}

void CrearArchivo(char* letras, char* tiempo, int* pistas)//Se crea la funcion CrearArchivo que guarda las estadisticas del simulador en memoria secundaria.
{

	FILE* archivo; //Se genera el puntero a archivo.
	archivo = fopen("Estadisticas.txt", "a");//Se abre en modo append.

	if (NULL == archivo)//Se verifica que se haya abierto o generado el archivo con exito.
	{
		fprintf(stderr, "No se pudo crear archivo %s.\n", "Estadisticas.txt");
		exit(-1);
	}
	else //En caso de que se lograra abrir con exito, imprime en el archivo las estadisticas.
	{
		fprintf(archivo, "--- Estadisticas del simulador ---\n");
		fprintf(archivo, "Cantidad de pistas: %s\n", "56");
		fprintf(archivo, "Agrupaciones que han caido por pista:\n");
		for (int i = 0; i < 56; i++)
		{
			_itoa_s(i, Pistas, 10);//Convierte a un string
			_itoa_s(pistas[i], Pistas2, 10);//Convierte a un string
			fprintf(archivo, "Pista-%s: %s\n", Pistas, Pistas2);
		}
		fprintf(archivo, "Tiempo de ejecucion: %s segundos\n", tiempo);
		fprintf(archivo, "Cantidad de letras nuevas pintadas: %s\n\n", letras);
	}
	fclose(archivo);//Se cierra el archivo al terminar.
}

void inicializar(int ocupado[56]) {//Inicializa en 0 una lista pasada.
	for (int i = 0; i < 56; i++) {
		ocupado[i] = 0;
	}
}

/*Dado un elemto tipo grupo, un array de enteros y una lista enlazada, generara primeramente parametros aleatorios para
el elemento tipo grupo. Ademas registrara la pista asignada a elemento, ademas agregar la posicion de la pista a la lista enlazada pistas ocupadas.*/
void datosAle(grupo& casca, int pistas[56], PtrTvalor& Lista) {
	bool aux = true;
	casca.colorActual = color();//Para el color simplemente se llama a la funcion que genera 1 de cuatro variantes.
	casca.contador = 0;//Contador siempre a 0;
	casca.draw = true;//Draw siempre true.
	casca.Y = (20 * (fastrand() % 10)) - 10;//El valor de y se genera utiliza un numero aleatorio pero manteniendo un rango.
	casca.limite = 500 + (20 * (fastrand() % 25));//El valor de limite se genera utiliza un numero aleatorio pero manteniendo un rango.
	//Para x necesitaremos un entero un puntero a un valor de tipo Tvalor y un while.
	int num;
	PtrTvalor Nuevo;
	while (aux) {
		num = fastrand() % 56;//Se genera un numero de 0 a 55;
		if (!estaEnLista(Lista, num)) {//Se consulta si este valor esta ocupado, en la lista enlazada de ocupados.
			Nuevo = CrearArticulo(num);//Si no esta ocupado se genera un nuevo Tvalor con el numero.
			AgregarInicio(Lista, Nuevo);//Y se agrega a la lista enlazada de ocupados
			aux = false;//Se termina el while
			pistas[num] += 1;//Utilzando el numero como index se incrementa en uno la posicion asociada a la pista.
		}
	}
	casca.X = num;//Finalmento se asigna el numero a x.
	inicializarstack(casca);//Se inicializa el stack con caracteres ' '.
}



int main()
{

	ALLEGRO_DISPLAY* pantalla;//Se genera un puntero a un elemento tipo display del allegro, esta para la simulacion.
	ALLEGRO_DISPLAY* pantalla2;//Genera un puntero a un display, para nuestro display con estadisticas.

	if (!al_init()) {//Se inicia y se verifica que el allegro se haya iniciado.
		fprintf(stderr, "No se puede iniciar allegro!\n");
		return -1;
	}

	al_set_new_display_flags(ALLEGRO_WINDOWED | ALLEGRO_RESIZABLE);//Se le otorga al display la capacidad de cambiar de tamaño.
	pantalla = al_create_display(1490, 900);//Se crea el display y se le otorga las dimenciones correspondientes.
	al_set_window_position(pantalla, 50, 50);//Se ubica el display en la pantalla.
	al_set_window_title(pantalla, "MATRIX");//Se le mote titulo a la ventana.

	if (!pantalla) {//Se verifica que el display se haya generado de manera correcta.
		fprintf(stderr, "No se puede crear la pantalla!\n");
		return -1;
	}

	//----------------------------------------------------------------------------------
	//Li­neas para obtener las funcionalidades del uso de las fuentes, teclado y audio
	al_init_font_addon();
	al_init_ttf_addon();
	al_init_image_addon();
	al_init_primitives_addon();
	al_install_keyboard();
	al_install_audio();
	al_init_acodec_addon();
	al_reserve_samples(7);
	al_install_mouse();

	//Punteros a elemetos de alegro para la musica y la fuente del texto.
	ALLEGRO_FONT* fuente2;
	ALLEGRO_FONT* fuente3;
	ALLEGRO_SAMPLE* musica;


	//Se crea una cola de eventos, y se le asigna un puntero de mismo tipo.
	ALLEGRO_EVENT_QUEUE* colaEventos = al_create_event_queue();

	//Inicializaciones de ALLEGRO
	fuente = al_load_font("Sheeping Dogs.ttf", 22, NULL);
	fuente2 = al_load_font("Sheeping Dogs.ttf", 40, NULL);
	fuente3 = al_load_font("Sheeping Dogs.ttf", 30, NULL);
	musica = al_load_sample("matrixSonido.mp3");

	//Timers que se necesitaran
	ALLEGRO_TIMER* segundoTimer = al_create_timer(1.0 / FPS);

	//Registro de los eventos
	al_register_event_source(colaEventos, al_get_timer_event_source(segundoTimer));
	al_register_event_source(colaEventos, al_get_keyboard_event_source());

	//Inicializacion del timer 
	al_start_timer(segundoTimer);
	//----------------------------------------------------------------------------------

	bool hecho = true; //Variable para el while principal de la simulacion.
	char caracter[1];  //Auxiliar para cuando se genera un nuevo caracter.
	int numGrupos[56];//Lista de enteros que se asocia a las pistas.
	double time_spent = 0.0; //Double que guardara el tiempo de ejecucion.
	int letras = 0; //Variable que contiene el numero de letras que se han impreso en pantalla.
	PtrTvalor ocupa; //Se genera un puntero de tipo Tvalor, este sera el puntero de origen de la lista enlazada.
	int const numCascadas = 25;  //Numero de cascadas/grupos de letras generados.
	grupo cascada[numCascadas];  //Se genera un array con elementos de tipo grupo, con un tamaño de numCascadas.

	InicializarInventario(ocupa); //Se inicializa la lista.
	inicializar(numGrupos); //Se inicializa la lista de enteros para las pistas poniendo todos los valores en 0.

	for (int i = 0; i < numCascadas; i++)//Se  recorre el array de elementos tipo grupo.
	{
		datosAle(cascada[i], numGrupos, ocupa); //Se les asignan nuevos valores por defectos a los parametros, se pasa el elemto a modificar, el numGrupos para sumar 1 cuando se seleccione la pista del grupo y el ocupe para buscar un numero que no este seleccionado.
	}

	al_play_sample(musica, 0.3, 0, 1, ALLEGRO_PLAYMODE_LOOP, NULL);//Se inicializa la musica del programa en bucle.

	clock_t begin = clock(); //A una variable tipo clock_t se le asigna una hora, esta sera el momento de comienzo de la simulacion. 

	while (hecho)
	{
		ALLEGRO_EVENT eventos; //Se genera un elemento evento del allegro.
		al_wait_for_event(colaEventos, &eventos);//Se asigana que se espere hasta que haya un evento en la cola de eventos, para pasarselo a evento, y continuar en el codigo.

		if (eventos.type == ALLEGRO_EVENT_KEY_DOWN) //En caso de que el evento sea el presionar una tecla.
		{
			switch (eventos.keyboard.keycode)
			{
			case ALLEGRO_KEY_ESCAPE: //Si se preciona ESCAPE se sale, para pasar a las estadisticas.
				hecho = false;
				break;
			}
		}
		if (eventos.type == ALLEGRO_EVENT_TIMER)//Si es el timer.
		{
			if (eventos.timer.source == segundoTimer)
			{
				for (int i = 0; i < numCascadas; i++) //Se recorrera el array de grupos de letras, donde cada grupo tiene dos estado dado por el parametro draw, donde dependiendo de si hay siperado el limite de posicion haran dos cosas.
				{
					if ((cascada[i].contador * 20) - 10 < cascada[i].limite && (cascada[i].contador * 20) - 10 < 950) { //Si no se ha superado el limite.
						if (cascada[i].draw) {//Si esta en modo dibujo.
							caracter[0] = GenerarRandom(); //Genera un valor aleatorio.
							letras++;//Se suma uno a los calores agregados.
							dibujar(caracter, cascada[i].carac, cascada[i].X, cascada[i].Y, cascada[i].colorActual);//Se dibuja el grupo pasando ademas el nuevo caracter que se le agregara.
							cascada[i].Y += 20;//Se mmovera la posicion en y, el equivalente a un movimiento hacia abajo.
							cascada[i].contador++;//Se le sumara al contador 1.
						}
						else { //Si esta en modo borrar
							dibujar2(cascada[i].X, cascada[i].Y);//Imprime un rectangulo negro en la posicion dada.
							cascada[i].Y += 20;//Realiza un movimiento hacia abajo.
							cascada[i].contador++;//Se le sumara al contador 1.
						}
					}
					else {//Se ha alcansado el limite.
						if (cascada[i].draw) { //Si enta en modo dibujo.
							cascada[i].Y -= cascada[i].contador * 20;//Devuelve la pocion a el valor de y inicial.
							cascada[i].contador = 0; //Restablese contador
							cascada[i].draw = false; //Cambia al estado borrar.
						}
						else {//Si alcanza el limite.
							Borrar(ocupa, cascada[i].X); //Borra el numero de pista del la lista enlazada de pistas ocupadas.
							datosAle(cascada[i], numGrupos, ocupa);//Se le otorgan nuevos datos al elemento.
						}
					}
					al_flip_display();
				}

			}
		}
	}
	al_destroy_display(pantalla); //Se destruye el display.
	al_destroy_sample(musica);  //Se destruye la musica.


	clock_t end = clock();//A una variable tipo clock_t se le asigna una hora, esta sera el momento de finalizacion de la simulacion. 
	time_spent += (double)(end - begin) / CLOCKS_PER_SEC;//Calcula el tiempo transcurrido encontrando la diferencia (end - begin) y dividiendo la diferencia por CLOCKS_PER_SEC para convertir a segundos.

	_itoa_s(time_spent, Tiempo, 10);//Convierte el tiempo de ejecucion a un string para el archivo.
	_itoa_s(letras, Letras, 10);//Convierte la cantidad de letras a un string para el archivo.

	//Se configura el display.
	al_set_new_display_flags(ALLEGRO_WINDOWED | ALLEGRO_RESIZABLE);
	pantalla2 = al_create_display(1000, 900);
	al_set_window_position(pantalla2, 400, 50);
	al_set_window_title(pantalla2, "Estadisticas");

	if (!pantalla2) {//Se verifica que el display se haya generado de manera correcta.
		fprintf(stderr, "No se puede crear la pantalla!\n");
		return -1;
	}

	//Se imprimen los elementos estaticos del display de estadisticas.
	al_draw_text(fuente2, al_map_rgb(100, 250, 250), 500, 50, ALLEGRO_ALIGN_CENTER, "Estadisticas");
	al_draw_text(fuente2, al_map_rgb(150, 150, 250), 500, 150, ALLEGRO_ALIGN_CENTER, "Cantidad de pistas:");
	al_draw_text(fuente3, al_map_rgb(150, 150, 250), 500, 200, ALLEGRO_ALIGN_CENTER, "56");
	al_draw_text(fuente2, al_map_rgb(150, 150, 250), 500, 300, ALLEGRO_ALIGN_CENTER, "Agrupaciones que han caido por pista: ");


	int j = 350;//Posicon en y del texto.
	int n = 0;//Posicion en x.
	for (int i = 0; i < 56; i++)//Recorrido para imprimir las estadisticas de las pistas.
	{
		_itoa_s(i, Pistas, 10);//Convierte a un string.
		//Imprime una pistas
		al_draw_text(fuente, al_map_rgb(150, 150, 250), 10.0 + (0 + n), j, ALLEGRO_ALIGN_LEFT, ("Pista-"));
		al_draw_text(fuente, al_map_rgb(150, 150, 250), 10.0 + (70 + n), j, ALLEGRO_ALIGN_LEFT, (Pistas));
		al_draw_text(fuente, al_map_rgb(150, 150, 250), 10.0 + (100 + n), j, ALLEGRO_ALIGN_LEFT, (":"));
		_itoa_s(numGrupos[i], Pistas, 10);//convierte a un string
		al_draw_text(fuente, al_map_rgb(150, 150, 250), 10.0 + (120 + n), j, ALLEGRO_ALIGN_LEFT, (Pistas));
		n = n + 170;//Se nueva la x a la derecha.
		if (n == 1020)//Si la x llega a 1020, se reinicai la misma a 0 y se le suma a y para bajar el y.
		{
			n = 0;
			j += 30;
		}
	}
	//Impresion del tiempo de ejecucion.
	al_draw_text(fuente2, al_map_rgb(150, 150, 250), 500, 700, ALLEGRO_ALIGN_CENTER, "Tiempo de ejecucion en segundos: ");
	al_draw_text(fuente3, al_map_rgb(150, 150, 250), 500, 750, ALLEGRO_ALIGN_CENTER, Tiempo);

	//IMpresion de la cantidad de letras pintadas.
	al_draw_text(fuente2, al_map_rgb(150, 150, 250), 500, 800, ALLEGRO_ALIGN_CENTER, "Cantidad de letras nuevas pintadas: ");
	al_draw_text(fuente3, al_map_rgb(150, 150, 250), 500, 850, ALLEGRO_ALIGN_CENTER, Letras);

	al_flip_display();

	bool estadisticas = true; //Booleano para el ciclo while de cerrado del programa.
	while (estadisticas) //Este while es igual que el de la simulacion solo que simplemente espera a que se presiones escape para terminar el programa.
	{
		ALLEGRO_EVENT eventos;
		al_wait_for_event(colaEventos, &eventos);

		if (eventos.type == ALLEGRO_EVENT_KEY_DOWN)
		{
			switch (eventos.keyboard.keycode)
			{
			case ALLEGRO_KEY_ESCAPE: //Si se preciona ESCAPE se sale
				estadisticas = false;
				break;
			}
		}
	}
	CrearArchivo(Letras, Tiempo, numGrupos); //Se crea o añade a un archivo el contenido de las estadisticas.

	DestruirInventario(ocupa); //Se destruye la lista enlazada.

	//Se destruyen los elementos de allegro restantes.
	al_destroy_display(pantalla2);
	al_destroy_timer(segundoTimer);
	al_destroy_font(fuente);
	al_destroy_font(fuente2);
	al_destroy_font(fuente3);
	al_destroy_event_queue(colaEventos);

}
