#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <regex.h>

char username[] = "mheld";
char realm[] = "marc@nu";

typedef struct{
  char* mac;
  int strength;
} pair;

#define BUFFER_SIZE     (256 * 1024) /* 256kB */

struct write_result {
  char *data;
  int pos;
};

static size_t curl_write( void *ptr, size_t size, size_t nmemb, void *stream) {
  struct write_result *result = (struct write_result *)stream;
  
  /* Will we overflow on this write? */
  if(result->pos + size * nmemb >= BUFFER_SIZE - 1) {
    fprintf(stderr, "curl error: too small buffer\n");
    return 0;
  }
  
  /* Copy curl's stream buffer into our own buffer */
  memcpy(result->data + result->pos, ptr, size * nmemb);
  
  /* Advance the position */
  result->pos += size * nmemb;
  
  return size * nmemb;
}

void formFor(char* out, pair* pair){
  //yeah, make this guy safe
  strcat(out, "<access-point><mac>");
  strcat(out, pair->mac);
  strcat(out, "</mac><signal-strength>");
  char strength[4];
  sprintf(strength, "%i", pair->strength);
  strcat(out, strength);
  strcat(out, "</signal-strength></access-point>");
}

void authFor(char* out, char* username, char* realm){
  strcat(out, "<?xml version='1.0'?><LocationRQ xmlns='http://skyhookwireless.com/wps/2005' version='2.6' street-address-lookup='full'><authentication version='2.0'><simple><username>");
  strcat(out, username);
  strcat(out,"</username><realm>");
  strcat(out, realm);
  strcat(out, "</realm></simple></authentication>");
}
// authFor + formFor + </LocationRQ>
void generateForm(char* out, pair* pairs[]){
  authFor(out, username, realm);

  while(*pairs){
    formFor(out, *pairs);
    pairs++;
  }

  strcat(out,"</LocationRQ>");
}

char* chop(char *string){
  int i, len;
  int new_i=0;
  len = strlen(string);
  char *newstring;
  
  newstring = (char *)malloc(len-1);
  
  for(i = 0; i < strlen(string)-1; i++){
    if(string[i] == ':'){
    
    }else{
      newstring[new_i] = string[i]; 
      new_i++;
    }
  }
  
  return newstring;
}

char* match(char* string){
  regex_t    preg;   
  char       *pattern = "<latitude>(.*)</latitude><longitude>(.*)</longitude>.*<street-number>(.*)</street-number><address-line>(.*)</address-line><city>(.*)</city><postal-code>(.*)</postal-code><county>(.*)</county><state.*>(.*)</state><country.*>(.*)</country>";
  int        rc;     
  size_t     nmatch = 11;
  regmatch_t pmatch[11];
  char* newstring;
  newstring = (char*) malloc(strlen(string)-1);
  
  if (0 != (rc = regcomp(&preg, pattern, REG_EXTENDED))) {
    printf("regcomp() failed, returning nonzero (%d)\n", rc);
    exit(EXIT_FAILURE);
  }
  
  if (0 != (rc = regexec(&preg, string, nmatch, pmatch, 0))) {
    printf("Failed to match '%s' with '%s',returning %d.\n",
	   string, pattern, rc);
  }else {
    sprintf(newstring, "location:");
    strncat(newstring, &string[pmatch[1].rm_so], pmatch[1].rm_eo - pmatch[1].rm_so);
    strcat(newstring, ",");
    strncat(newstring, &string[pmatch[2].rm_so], pmatch[2].rm_eo - pmatch[2].rm_so);
    strcat(newstring, "\naddress: ");
    strncat(newstring, &string[pmatch[3].rm_so], pmatch[3].rm_eo - pmatch[3].rm_so);
    strcat(newstring, " ");
    strncat(newstring, &string[pmatch[4].rm_so], pmatch[4].rm_eo - pmatch[4].rm_so);
    strcat(newstring, ". ");
    strncat(newstring, &string[pmatch[5].rm_so], pmatch[5].rm_eo - pmatch[5].rm_so);
    strcat(newstring, ", ");
    strncat(newstring, &string[pmatch[8].rm_so], pmatch[8].rm_eo - pmatch[8].rm_so);
    strcat(newstring, " ");
    strncat(newstring, &string[pmatch[6].rm_so], pmatch[6].rm_eo - pmatch[6].rm_so);
    strcat(newstring, " ");
    //county? really?
    //    strncat(newstring, &string[pmatch[7].rm_so], pmatch[7].rm_eo - pmatch[7].rm_so);
    strncat(newstring, &string[pmatch[9].rm_so], pmatch[9].rm_eo - pmatch[9].rm_so);
    strcat(newstring, "\n");

  }
  regfree(&preg);

  return newstring;

}


int main(int argc, char *argv[]){
  int mac_count = 0;

  //TODO: need to make this number non-static
  char* macs[20];

  //
  //START get macs from system
  //

  FILE *fp;
  char line[19]; // maximum line size -- 6 2-chars + a couple ":"s

  fp = popen("cat iwlist2 | grep -Eo [:0-9A-F:]{2}\\(\\:[:0-9A-F:]{2}\\){5}", "r");
  //  fp = popen("iwlist scan | grep -Eo [:0-9A-F:]{2}\(\:[:0-9A-F:]{2}\){5}", "r");

  if(fp == NULL){
    printf("something screwed up");
    return 0;
  }
  
  // read the fp line by line into macs
  // expecting a file filled with "MA:CA:DD:RE:SS:SS\n"s
  while( fgets(line, sizeof(line), fp) ){
    macs[mac_count] = chop(line); //then set it
    mac_count++;
  }

  pclose(fp);

  //
  //END get macs from system
  //

  pair* pairs[mac_count];
  int count;
 
  // for( count = 1; count < argc; count++ ){
  for( count = 1; count < mac_count; count++ ){
    pair *p; 
    p = malloc(sizeof(pair));
    //    p->mac = argv[count]; 
    p->mac = macs[count];
    p->strength = -50;
    pairs[count - 1] = p;
  }

  /* set up initial data */
  CURL *curl = curl_easy_init(); 
  char *data;

  /* Create the write buffer */
  data = malloc(BUFFER_SIZE);
  if (! data){
    fprintf(stderr, "Error allocating %d bytes.\n", BUFFER_SIZE);
  }

  struct write_result write_result = {
    .data = data,
    .pos = 0
  };
  
  char form[500000];  
  generateForm(form, pairs);

  /* Set curl's parameters */
  struct curl_slist *headers=NULL; 
  headers = curl_slist_append(headers, "Content-Type: text/xml");
  curl_easy_setopt(curl, CURLOPT_URL, "https://api.skyhookwireless.com/wps2/location");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &write_result);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, form);

  curl_easy_perform(curl);

  /* null terminate the string */
  data[write_result.pos] = '\0';

  /* Print what we got. can't use printf because the data is too big */
  //  fwrite(data, write_result.pos, sizeof(char), stdout);

  // grab what data we need from the xml response
  data = match(data);

  printf("%s", data);

  /* Also print to temp file */
  FILE *loc_file; 
  loc_file = fopen("/tmp/location","w");
  fwrite(data, strlen(data), sizeof(char), loc_file);
  fclose(loc_file);

  /* Don't forget to free your memory! */
  free(data);
  //free(pairs);
  curl_easy_cleanup(curl);
  curl_global_cleanup();
  
  return 0;
}
