#include<bits/stdc++.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <pwd.h>
#include <grp.h>
using namespace std;

vector<string> filelist;
vector<string> fullfilelist;
vector<string> back_store;
vector<string> forward_store;
vector<string> command_tokens;
char *home;
string currPath;
string currDir;
string input_command;
int term_row, term_column;
bool searchflag=false;
int cx=1, cy=1;
int curr_window=0;
int content_count;

//****************Fuunction declarations*********//
void display(string dirName, string dn);
void up(); void down(); void left(); void right(); void enter(); void backspace(); void homekey();
void setbackpath(string s);

//****Utility functions for command mode******//
string pathcreation(string s); void split_input(string s); void clearlastcmd(); int commandmode();
void performaction(); void throwerror(); void delete_dir_helper(string path);
void filecopy_helper(string source, string destination); void dircopy_helper(string source, string destination);
void search_helper(string path, string name);

//****functions implemented in command mode*****//
void _rename(); void create_dir(); void create_file(); void delete_dir(); void delete_file();
void _copy(); void _move(); void _goto(); void search();


//***********Function to set terminal row size and terminal column size*******//
void set_scroll_indices()
{
    struct winsize win_parameter;
    ioctl(STDOUT_FILENO,TIOCGWINSZ, &win_parameter);
    term_row=win_parameter.ws_row-2;
    //term_column=win_parameter.ws_col;
}

//************function to get current row size*********//
int get_scroll_indices()
{
    return (content_count<=term_row)?(content_count-1):(term_row+curr_window);
}

//******Function that takes a file/directory name and returns its fullpath
string getfullpath(string dname)
{
    string path= currPath;
    //cout<<"curpath="<<currPath;
    path.append("/");
    path.append(dname);
    return path;
}

//***********sets the path 1 backward to previous directory***********//
void setbackpath(string path)
{
    size_t pos;
    pos=path.find_last_of("/\\");
    currPath=path.substr(0,pos);
}


//**********Function to display updated list according to terminal size****************//

void updateList()
{
    set_scroll_indices();
    printf("\033[H\033[J");
   /* printf("%c[%d;%dH", 27, term_row+2, 1);
    printf("---Normal Mode---");
    printf("\033[1;1H"); */

    cy=1;
    printf("\033[%d;%dH",cx,cy);
    int from = curr_window;                            
    int to = get_scroll_indices();
    for(int i=from; i<=to; ++i)
        display(fullfilelist[i],filelist[i]);
}

/* Function to open a directory, store its contents and list them*/
void openDirectory(const char *path)
{
    filelist.clear();
    fullfilelist.clear();
    content_count=0;
    //set_scroll_indices();
    //printf("\033[H\033[J");                          //clear screen
    //printf("%c[%d;%dH", 27, term_row+2, 1);
    //printf("---Normal Mode---");
    //printf("\033[1;1H");          //position cursor at leftmost corner of screen to begin printing
    DIR *dp;
    struct dirent *file;
    dp=opendir(path);
    if(!dp)
    {
        fprintf(stderr,"cannot open directory \n");
        return;
    }
    //chdir(path);
     while((file=readdir(dp))!=NULL)
    {
        if(string(file->d_name)==".." && strcmp(path,home)==0)  //restrict access beyond root in which application was opened
            continue;
        else
        {
            filelist.push_back((string)file->d_name);
            //string s=getfullpath((string)file->d_name);
            string str=(string)path;
            str.append("/");
            str.append(file->d_name);
            fullfilelist.push_back(str);
        }    
    }
    sort(fullfilelist.begin(), fullfilelist.end());
    sort(filelist.begin(),filelist.end());
    closedir(dp);
    content_count=fullfilelist.size();
    updateList();
    printf("\033[1;1H");
}

/*display contents of a directory*/
void display(string dirName, string dn)
{
    struct stat info;
    struct passwd *pd;
    struct group *gp;
    char *lastmodified;
    stat(dirName.c_str(),&info);
    printf("%-50s",dn.c_str());
    printf("\t");
    printf((S_ISDIR(info.st_mode)) ? "d" : "-");
	printf((info.st_mode & S_IRUSR) ? "r" : "-");
	printf((info.st_mode & S_IWUSR) ? "w" : "-");
	printf((info.st_mode & S_IXUSR) ? "x" : "-");
	printf((info.st_mode & S_IRGRP) ? "r" : "-");
	printf((info.st_mode & S_IWGRP) ? "w" : "-");
	printf((info.st_mode & S_IXGRP) ? "x" : "-");
	printf((info.st_mode & S_IROTH) ? "r" : "-");
	printf((info.st_mode & S_IWOTH) ? "w" : "-");
	printf((info.st_mode & S_IXOTH) ? "x" : "-");
    pd = getpwuid(info.st_uid);
	gp = getgrgid(info.st_gid);
    (printf("\t\t%-8s ",pd->pw_name));
    (printf("\t%-8s ",gp->gr_name));
    (printf("\t%8ld \t",info.st_size));
    lastmodified=ctime(&info.st_mtime);
    int n=strlen(lastmodified);
    //std::cout<<lastmodified[n-1];
    for(int i=0; i<n-1; ++i)
        (printf("%c",lastmodified[i]));
    printf("\n");
}
//****Start to explore the files and folders***
void explore()
{  
    //printf("\033[1;1H");
    struct termios current_attr, new_attr;
    tcgetattr(STDIN_FILENO, &current_attr);
    new_attr=current_attr;                            //to restore later
    new_attr.c_lflag&=~ICANON;                       //disable cannonical mode
    new_attr.c_lflag&=~ECHO;                         //disable  echoing
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_attr);
    while(1)
    {
        /*When we press up/down/back/forward keys, their corresponding ASCII values consist of 3 characters. 
          Ex: A : 27 91 65 (the integer values)
        */
        
        set_scroll_indices();
        //printf("\033[H\033[J");
        printf("%c[%d;%dH", 27, term_row+2, 1);
        printf("---Normal Mode---");
        printf("\033[%d;%dH",cx,cy);


        int ch = cin.get();            //test the first input
			if (ch == 27)
			{
				ch = cin.get();       //skip the middle input
				ch = cin.get();       //store the last input in ch and compare
                if(ch==65)            //if UP arrow key is pressed
                {
                    up();

                }
                if(ch==66)
                {
                    down();         //if DOWN arrow key is pressed
                }
                if(ch==67)
                {
                    right();       //if RIGHT arrow key is pressed
                }
                if(ch==68)
                {
                    left();        //if LEFT arrow key is pressed
                }
            }
            if(ch==10)
            {
                enter();
            }
            if (ch == 'h')         //home key is pressed
			{
				homekey();
				
			}
			if (ch == 127)    //backspace is pressed
			{
				backspace();
			}
            if(ch=='k')
                up();
            if(ch=='l')
                down();
            if(ch==58)        // : pressed
            {
                int r = commandmode();
                if(r==0)
                    openDirectory(currPath.c_str());
            }
            if(ch=='q')
                break;
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &current_attr);    //restore to previous attributes
}

//Creating a fullpath of a string for command mode
string pathcreation(string s)
{
    string abspath="";
    if(s[0]=='/')
        abspath=string(home) +s;
    else if(s[0]=='.')
        abspath=currPath+"/"+ s.substr(1,s.length());
    else if(s[0]=='~')
        abspath=string(home) + s.substr(1,s.length()-1);
    else
        abspath=currPath+"/"+s;
    return abspath;
}


//****Function to extract words from given input command
void split_command(string s)
{
    command_tokens.clear();
	for(int i=0; i<s.size(); i++)
    {
		string temp;
		while(s[i]!=' ' && i<s.size())
        {
			if(s[i]=='\\')                    //to take care of filename with spaces (it should be entered as "file\ name")
            {
				temp+=" ";
				i+=2;
			}
			else{
				temp+=s[i];
				i++;
			}
		}
		command_tokens.push_back(temp);
	}
    for(auto a : command_tokens) cout<<a<<"*";
	return;
}

//Clears the last command for next command to be printed
void clearlastcmd()
{
	printf("\033[%d;%dH",term_row+2,1);
	printf("%c[2K", 27);
	cout<<":";
}

//Function to print error in status bar
void throwerror(string s)
{
	clearlastcmd();
	cout<<"\033[0;31m"<<s<<endl;
	cout<<"\033[0m";
	cout<<":";
}


//***Function to start command mode***//

int commandmode()
{
    //clearlastcmd();
    char ch=0;
    while(ch!=27)
    {
        clearlastcmd();
        input_command.clear();
        while((ch=getchar())!=10 && ch!=27)     //keep getting character until enter is pressed
        {
            if(ch==127)        //backspace is pressed
            {
                if(input_command.size()==0)
                    clearlastcmd();
                else if(input_command.size()==1)
                {
                    input_command.clear();
                    clearlastcmd();
                }
                else
                {
                    input_command=input_command.substr(0,input_command.size()-1);
                    clearlastcmd();
                    cout<<input_command;
                }
                
            }
            else
            {
                input_command+=ch;
                cout<<ch;
            }
            
        }
        split_command(input_command);
        performaction();
    }
    return 0;
}


//****Function to perform various commands in command mode****//
void performaction()
{
    if (command_tokens[0] == "rename")
        _rename();
    else if (command_tokens[0] == "create_file")
        create_file();
    else if (command_tokens[0] == "create_dir")
        create_dir();
    else if (command_tokens[0] == "delete_file")
        delete_file();
    else if (command_tokens[0] == "delete_dir")
        delete_dir();
    else if (command_tokens[0] == "copy")
        _copy();
    else if (command_tokens[0] == "move")
        _move();
    else if (command_tokens[0] == "goto")
        _goto();
    else if (command_tokens[0] == "search")
        search();
    else 
    {
        throwerror("Command not found");
    }
    return;
}




int main()
{
    char buff[FILENAME_MAX];
    home = getcwd( buff, FILENAME_MAX );
    cout<<home;
    currPath=string(home);
    currDir=".";
    //cout<<home;
    openDirectory(home);                                  //display present directory
    explore();                                      //start navigation
   // cout<<currPath;
    exit (0);    
}


//************* Function to navigate upwards ************//

void up()
{
    if(cx>1)
    {
        cx--;
        printf("\033[%d;%dH",cx,cy);      
    }
    else if(cx==1 && (cx+curr_window) > 1)
    {
        curr_window-=1;
        updateList();
        printf("\033[%d;%dH",cx,cy);
    }
}


//************* Function to navigate downwards ************//

void down()
{
  if(cx<=term_row && cx<content_count)                //0 1 2 3 4 5 6 7 8 9
  {
      cx++;
      printf("\033[%d;%dH",cx,cy);
  }
  else if(cx>term_row  && cx+curr_window <content_count)
  {
      curr_window++;
      updateList();
      printf("\033[%d;%dH",cx,cy);
  }
}


//************* Function to enter a directory ************//

void enter()
{
    currDir=filelist[cx+curr_window-1];
    string fullpath;
    //fullpath=currPath+"/"+currDir;
    fullpath=getfullpath(currDir);
    struct stat sb;
    stat(fullpath.c_str(), &sb);
    if((sb.st_mode & S_IFMT)==S_IFDIR)
    {
        cx=1;
        //searchflag=false;
        if(currDir==".")
        {
            printf("\033[1;1H");
        }
        else if(currDir=="..")                                                     
        {
            back_store.push_back(currPath);
            forward_store.clear();
            setbackpath(currPath);                  
        }
        else
        {
            back_store.push_back(currPath);
            forward_store.clear();
            currPath=fullpath;
        }
        openDirectory(currPath.c_str());
    }
    else if ((sb.st_mode & S_IFMT) == S_IFREG)
	{
		int fileOpen=open(fullpath.c_str(),O_WRONLY);
		dup2(fileOpen,2);
		close(fileOpen);
		pid_t processID = fork();
		if(processID == 0)
		{
			execlp("xdg-open","xdg-open",fullpath.c_str(),NULL);
			exit(0);
		} 
	}
	else
	{
		cout<<"Unknown File !!! :::::"<<string(currDir);
	}
    //printf("\033[1;1H");
}


//************* Function to go back ************//

void left()
{
    if(!back_store.empty())
    {
        forward_store.push_back(currPath);                       
        string top = back_store[back_store.size()-1];
        back_store.pop_back();
        currPath=top;
        openDirectory(currPath.c_str());
        cx=1; cy=1;
        printf("\033[%d,%dH",cx,cy);
    }
}


//************* Function to go forward ************//

void right()
{
    if(!forward_store.empty())
    {
        back_store.push_back(currPath);                             
        string top=forward_store[forward_store.size()-1];
        forward_store.pop_back();                                      
        currPath=top;
        openDirectory(currPath.c_str());
        cx=1; cy=1;
        printf("\033[%d,%dH",cx,cy);
    }
}


//************* Function to go to root of application ************//

void homekey()
{
    if(currPath!=string(home))
	{
		back_store.push_back(currPath);
		forward_store.clear();
		currPath=string(home);
		openDirectory(currPath.c_str());
		cx = 1, cy = 1;
		printf("\033[%d;%dH",cx,cy);
	}
}


//************* Function to go back to parent directory ************//

void backspace()
{
    if ((strcmp(currPath.c_str(), home) != 0))
	{
		back_store.push_back(currPath);
		forward_store.clear();
		setbackpath(currPath);
		openDirectory(currPath.c_str());
		cx = 1, cy = 1;
		printf("\033[%d;%dH",cx,cy);
	}
}

//************* Function to go rename a file or directory ************//

void _rename()
{
    if(command_tokens.size()!=3)
    {
        throwerror("Wrong number of arguments");
    }
    else
    {
        if(rename(command_tokens[1].c_str(), command_tokens[2].c_str())!=0)
            throwerror("Couldn't rename");
    }
    
}

//************* Function to create a file ************//

void create_file()
{
    if(command_tokens.size()!=3)
        throwerror("Wrong number of arguments");
    else
    {
        string destination = pathcreation(command_tokens[2]);
        string filename = destination + "/" + command_tokens[1];
        struct stat sb;
        stat(destination.c_str(),&sb);
        if(!S_ISDIR(sb.st_mode))
            throwerror("Destination is not a directory");
        else
        {
            int exitcode=open(filename.c_str(),O_RDONLY | O_CREAT,S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH ); 	
		    if (exitcode == -1)
	        {
			    throwerror("Error in creating new file " + string(filename));	       
	        }
        }
    }
}

//************* Function to create a directory ************//

void create_dir()
{
    if(command_tokens.size()!=3)
        throwerror("Wrong number of inputs!!");
    else
    {
        string destination = pathcreation(command_tokens[2]);
        string dirname = destination + "/" +command_tokens[1];
        int exitcode =mkdir(dirname.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if (exitcode == -1)
	        {
			    throwerror("Error in creating new folder " + string(dirname));	       
	        }
    }    
}

//************* Function to delete a file ************//

void delete_file()
{
    if(command_tokens.size()!=2)
        throwerror("Wrong number of inputs!!");
    else
    {
        string path = pathcreation(command_tokens[1]);
        if ((remove(path.c_str()))!=0)
	        {
			    throwerror("Error in deleting file " + string(path));	       
	        }
    }  
}

//************* Function to delete a directory ************//

void delete_dir()
{
    if(command_tokens.size()!=2)
        throwerror("Wrong number of arguments!!");
    else
    {
        string path = pathcreation(command_tokens[1]);
        struct stat sb;
        if(stat(path.c_str(),&sb)!=0)
        {
            throwerror("stat error");
            return;
        }
        if(S_ISDIR(sb.st_mode))
            delete_dir_helper(path);
        else
            throwerror("not a directory");
    }    
}

//*** Helper function to delete directory recursively****//
void delete_dir_helper(string path)
{
    DIR *d;
    struct dirent *dir;
    d=opendir(path.c_str());
    if(!d)
    {
        throwerror("opendir malfunction");
        return;
    }
    else
    {
        while((dir=readdir(d))!=NULL)
        {
            if((string)(dir->d_name)=="." || (string)(dir->d_name)=="..")
                continue;
            string destination = path + "/" + (string)dir->d_name;
            struct stat sb;
            if(stat(destination.c_str(),&sb)!=0)
            {
                throwerror("stat error inside helper");
                return;   
            }
            if(S_ISDIR(sb.st_mode))
                delete_dir_helper(destination);
            else
                if ((remove(destination.c_str()))!=0)
	        {
			    throwerror("Error in removing file " + string(destination));	       
	        }
        }
        int exitcode = rmdir(path.c_str());
        if(exitcode!=0)
            throwerror("Error in removing directory: " + path);
        closedir(d);
    }
    
}


//***Helper function to copy files to a directory***//

void filecopy_helper(string source, string dest)
{
    int s, d;
    char buffer[1024];
    long long size;

    if(((s= open(source.c_str(), O_RDONLY))==-1) ||((d=open(dest.c_str(),O_CREAT|O_WRONLY,S_IRUSR| S_IWUSR| S_IXUSR)) == -1))
    {    throwerror("error in opening file" + source + " or " + dest); return; }

    while((size=read(s, buffer, 1024)) > 0)
    {
        if(write(d, buffer, size) != size)
        {    throwerror("error in copying"); return; } 
    }

    struct stat sb;
    if(stat(source.c_str(), &sb)==-1)
        {
            throwerror("stat error for source"); return;
        }
    else
    {
        if(chown(dest.c_str(), sb.st_uid, sb.st_gid)!=0)
        {
            throwerror("Error in chown for setting ownership of file");
            return;
        }
        if(chmod(dest.c_str(), sb.st_mode)!=0)
        {
            throwerror("Error in chmod for setting permission of file");
            return;
        }
    }
}

//***Helper function to copy directories***//

void dircopy_helper(string source, string dest)
{
    DIR* d;
    d = opendir(source.c_str());
    struct dirent* dir;
    if(!d)
    {
        throwerror("opendir malfunction for "+source);
        return;
    }

    while((dir=readdir(d))!=NULL)
    {
        if((string)(dir->d_name)=="." || (string)(dir->d_name)=="..")
            continue;
        else
        {
            string from_path = source + "/" + (string)(dir->d_name);
            string to_path = dest + "/" + (string)(dir->d_name);
            struct stat sb;
		    if (stat(from_path.c_str(),&sb) == -1)
            {
		        throwerror("stat error");
                return;
		    }
		    if((S_ISDIR(sb.st_mode)))
            {
                if(mkdir(to_path.c_str(),S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IXOTH)!=0)
                {
                    throwerror("mkdir error in" + to_path); return;
                }
                else
                    dircopy_helper(from_path,to_path);
            }
            else
			    filecopy_helper(from_path,to_path);
        }
    }
    closedir(d);
}

//************* Function to copy files or directories************//

void _copy()
{
    if (command_tokens.size() < 3)
        throwerror("too few arguments");
    else
    {
        string dest_folder=pathcreation(command_tokens[command_tokens.size()-1]);
         struct stat sb;
        if(stat(dest_folder.c_str(),&sb)!=0)
        {
            throwerror("stat error in destination");
            return;
        }
        if(!S_ISDIR(sb.st_mode))
        {
            throwerror("Destination is not a directory"); return;
        }
        for (unsigned int i = 1; i < command_tokens.size() - 1; ++i)
        {
            string from_path = pathcreation(command_tokens[i]);
            size_t pos = from_path.find_last_of("/\\");
            string filename=from_path.substr(pos+1,from_path.length()-pos);
            string to_path = dest_folder + "/" + filename;
            struct stat buff;
            if(stat(from_path.c_str(),&buff)==-1)
            {
                throwerror("stat error"); 
                return;
            }
            if(S_ISDIR(buff.st_mode))
            {
                if(mkdir(to_path.c_str(),0777!=0))
                {
                    throwerror("error in making directory "+to_path);
                }
                else
                {
                    dircopy_helper(from_path, to_path);
                }
                
            }
            else
                filecopy_helper(from_path, to_path);
        }
    }
    return;
}

//************* Function to move ************//

void _move()
{
    _copy();
    command_tokens.pop_back();
    for(int i=1; i<command_tokens.size(); ++i)
    {
        string fullpath=pathcreation(command_tokens[i]);
        struct stat sb;
        if(stat(fullpath.c_str(),&sb)!=0)
        {
            throwerror("stat error in file/folder");
            return;
        }
        if(S_ISDIR(sb.st_mode))
        {
            delete_dir_helper(fullpath);
        }
        else
        {
            int exitcode = remove(fullpath.c_str());
            if(exitcode==-1)
                throwerror("cannot remove file"+fullpath);
        }
        
        
    }
}

//************* Function to search ************//

void search_helper(string path, string name)
{
    DIR *d;
    struct dirent *dir;
    d=opendir(path.c_str());
    if(!d)
    {
        throwerror("error in opening current path"+path);
    }
    else
    {
        while(dir=readdir(d))
        {
            if(((string)(dir->d_name)==".") || ((string)(dir->d_name)=="..") )
                continue;
            else
            {
                if(name==dir->d_name)
                {
                    throwerror("TRUE!!!"); searchflag=true;
                    return;
                }
                else
                {
                    struct stat sb;
                    string newpath= path+ "/" + dir->d_name;
                    stat(newpath.c_str(),&sb);
                    if(S_ISDIR(sb.st_mode))
                        search_helper(newpath,name);
                    else
                    {
                        continue;
                    }   
                }   
            }
        }
        closedir(d);
    }
}

void search()
{
    search_helper(currPath, command_tokens[1]);
    if(searchflag==false)
        throwerror("FALSE!!!");
}

//********Function to go to**************//
void _goto()
{
    printf("\033[%d;%dH",cx,cy);
    size_t pos = command_tokens[1].find_last_of("/\\");
    string filename=command_tokens[1].substr(pos+1,command_tokens[1].length()-pos);
    currDir=filename;
    //throwerror(currDir);
    forward_store.clear();
    back_store.push_back(currPath);
    currPath=command_tokens[1];
    openDirectory(currPath.c_str());
}