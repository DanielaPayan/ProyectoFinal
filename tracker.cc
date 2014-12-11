#include <iostream>
#include <czmq.h>
#include <thread> 
#include <unordered_map>
#include <set>
#include <vector>
#include <utility>


using namespace std;
/*  En nodoSS, guarda las canciones y sus respectivas partes que tiene un nodo.
	Ademas guardo la direccion donde el nodo recibe las solicitudes y su estado.	
*/
unordered_map<zframe_t*,pair < unordered_map<string,vector<string>>, pair<string,bool > > > nodoSS; //nodo:canciones. idnodo,cancion,vector<partes>
/*  En songP, registro todas las canciones que hay en el sistema y sus respectivas partes.

*/
unordered_map<string,vector<string>> songP;//song:partes
unordered_map<zframe_t*,string> nodoAddress;

//eliminar cada registro.
void desconectar(zframe_t* id){
	cout<<"El error esta aca"<<endl;
	for ( auto it = nodoSS.begin(); it != nodoSS.end(); ++it ){
		zframe_t* idN= it->first;
	    zframe_t* a=zframe_dup(idN);
		if(zframe_eq (id, a))
			nodoSS.erase(a);
		cout<<"El error no esta aca"<<endl;
	}
}


/*  handleNodeMessage, maneja el mensaje que llega desde un nodo.
	Tres tipos: register,query y update.
*/
void handleNodeMessage(zmsg_t* msg,zmsg_t* outmsg,void* nodos){
	zframe_t* idN=zmsg_pop(msg);
	string code(zmsg_popstr(msg)); 
	if(code.compare("register")==0){
		string address(zmsg_popstr(msg));
		//zframe_t* dup=zframe_dup(idN);
		int nsong=atoi(zmsg_popstr(msg));
		for (int i = 0; i < nsong;i++){ //Itero por cada cancion del nodo
			string song(zmsg_popstr(msg));
			int npsong=atoi(zmsg_popstr(msg));
			vector<string> s; //partes
			for (int j = 0; j < npsong;j++){  //Saco las partes de una cancion
				string p(zmsg_popstr(msg));
				s.push_back(p);
			}
			zframe_t* copy=zframe_dup(idN);
			zframe_t* copy2=zframe_dup(idN);
			//zframe_print(copy2,"Este id fue registrado en nodoSS");

			songP[song]=s; //Registro global de las canciones del sistema


			nodoSS[idN].first[song]=s;
			nodoSS[idN].second.first=address;
			nodoSS[idN].second.second=1;
			//cout<<"Tamaño de nodoSS:"<<nodoSS.size()<<endl;
		}					
	}else if(code.compare("query")==0){
		string song(zmsg_popstr(msg));
		zmsg_addstr(outmsg,song.c_str()); //song
		zmsg_addstr(outmsg,to_string(songP[song].size()).c_str()); //npart totales de una cancion
		for ( auto it = nodoSS.begin(); it != nodoSS.end(); ++it ){ //Recorro tabla hash para bsucar la cancion
			//cout<<"NUMERO DE VECES QUE ENTRO AQUI"<<endl;
			//cout<<"Tamaño de nodoSS:"<<nodoSS.size()<<endl;
	      	zframe_t* idNN= it->first;
	      	zframe_t* a=zframe_dup(idNN);
	      	//zframe_print(a,"Id nodo, deberian ser solo 2");

	      	pair < unordered_map<string,vector<string>>, pair<string,bool > > sp=it->second; //songs/parts - address/state
	      	if(sp.second.second==1&& sp.first.count(song)>0){
	      		zmsg_addstr(outmsg,sp.second.first.c_str()); //dirNodo
	      		int lp=sp.first[song].size(); // Cantidad de partes de la cancion que tiene este nodo
	      		zmsg_addstr(outmsg,to_string(lp).c_str()); //nparts
	      		for (int i = 0; i <lp;i++){
	        		zmsg_addstr(outmsg,sp.first[song][i].c_str()); //nparts
	       		}
	      	}
	      	   	
	    }
	    cout<<"Salida del handleNodeMessage"<<endl;
	    zmsg_addstr(outmsg,"end");
	    zframe_t* dup=zframe_dup(idN);
	    zmsg_prepend(outmsg,&dup);
	    zmsg_print(outmsg);
	    zmsg_send(&outmsg,nodos);
	}else if(code.compare("update")==0){
		string song(zmsg_popstr(msg));
		string part(zmsg_popstr(msg));
		//zframe_t* dup=zframe_dup(idN);
		string dir(zmsg_popstr(msg));
		if(nodoSS.count(idN)>0){  //Si el id esta registrado en la tabla hash
			if(nodoSS[idN].first.count(song)>0) //si el nodo tiene un registro de la cancion
				nodoSS[idN].first[song].push_back(part);
			else{
				vector<string> s;
				s.push_back(part);
				nodoSS[idN].first[song]=s;
			}
		}else{
			vector<string> s;
			s.push_back(part);
			nodoSS[idN].first[song]=s;
			//zframe_print(dup,"Id del Nodo");
			nodoSS[idN].second.first=dir;
			//cout<<"Uodate si no esta registrada: "<<dir<<endl;
			nodoSS[idN].second.second=1;
		}
	}else if(code.compare("desconectar")==0){
		zframe_t* idNN=zframe_dup(idN);
		nodoSS[idN].second.second=0;
		cout<<"Error";
		desconectar(idN);
		zmsg_t* msgnodo=zmsg_new();
		zmsg_append(msgnodo,&idNN);
		zmsg_addstr(msgnodo,"OK");
		zmsg_send(&msgnodo,nodos);
	}
}





int main(int argc, char** argv){


	// ./tracker puertoNodo
	zctx_t *context = zctx_new ();

	void* nodos = zsocket_new(context,ZMQ_ROUTER);
  	int nodoPort=zsocket_bind(nodos, "tcp://*:%s",argv[1]);
  	cout << "Listen to Nodes: "<< "localhost:" << nodoPort << endl;

  	zmq_pollitem_t items[] = {{nodos, 0, ZMQ_POLLIN, 0}};
  	while (true) {
	    zmq_poll(items, 1, 10 * ZMQ_POLL_MSEC);
	    if (items[0].revents & ZMQ_POLLIN) {
	      	cerr << "From nodo\n";
	      	zmsg_t* msg=zmsg_recv(nodos);
	  		zmsg_print(msg);
	      	zmsg_t* outmsg = zmsg_new();
	      	handleNodeMessage(msg,outmsg,nodos);
	    }     
  	}

	return 0;
}
