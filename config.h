/*
 * Simple library for handling the config files.
 * The format of cfg file is as follow
 *
 * settings.cfg:
 * # This is a comment
 * key=value
 * key1=value1 #comment can also be here
 * 
 */

#ifndef CONFIG_H
#define CONFIG_H

/*
 * Load config from files
 * @param cname : config file name
 * @return 		: 0 for successfully load
 * 				: negative for failed to load
 */
int config_open(char *cname);

/*
 * Always unload the resource when finish loading configs
 */
void config_close();

/*
 * Get the value of corresponding key
 * @param key 	: input key for searching
 * @param value : return value
 * @return		: 1 for found
 * 				: 0 for not found
 */
int config_get_value(char *key, char *value);

#endif
