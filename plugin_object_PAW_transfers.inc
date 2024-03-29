<?php

/**
 * @file
 * Plugin for the Islandora BagIt Drupal module.
 *
 * Creates Bags compatible with Archivematica (http://archivematica.org)
 * transfer Bags by creating 'metadata' and 'objects' subdirectories. 
 * Moves OBJ and other archival datastreams into the 'objects' directory.
 * Moves unique metadata datastreams into the 'metadata' directory.
 *
 * Adds a PREMIS metadata file as generated by the Islandora PREMIS module.
 */

/**
 * Returns an array of source and destination file paths.
 *
 * @param object $islandora_object
 *     The Islandora object to create a Bag for.
 *
 * @param string $tmp_ds_directory
 *     The temporary directory where the datastream files have been downloaded.
 *
 * @return array|bool
 *     An array of source and destination file paths, or FALSE
 *     if no datastream files are present.
 */
function islandora_bagit_plugin_object_PAW_transfers_init($islandora_object, $tmp_ds_directory) {
    // In this plugin, we want to exclude most derivatives and redundant datastreams
    $obj = array( 'OBJ', 'PDFA' );
    $md  = array( 'HOCR', 'MADS', 'MEDIATRACK', 'MODS', 'OCR', 'POLICY',
                'RELS-EXT', 'RELS-INT', 'TECHMD', 'TRANSCRIPT' );

    $files_to_add = array();
    $ds_files = islandora_bagit_retrieve_datastreams($islandora_object, $tmp_ds_directory);

    // Create a subdirectory in $tmp_ds_directory.
    mkdir($tmp_ds_directory . '/objects', 0777, TRUE);
    mkdir($tmp_ds_directory . '/metadata', 0777, TRUE);

    // Loop through the files and move them into either the 'objects' or 'metadata' subdirectory.
    $objects = 0;
    foreach ($ds_files as $ds_filename) {
        if (file_exists($ds_filename) && is_file($ds_filename)) {
            $parts = explode('.', basename($ds_filename));
            if (in_array( $parts[0], $obj )) {
                $basename = basename($ds_filename);
                // move file to objects subdirectory
                rename($ds_filename, $tmp_ds_directory . '/objects/' . $basename);
                // Add the file to $files_to_add array
                $files_to_add[] = array(
                    'source' => $tmp_ds_directory . '/objects/' . $basename,
                    'dest' => 'objects/' . $basename,
                );
                $objects++;
            } else if (in_array( $parts[0], $md )) {
                $basename = basename($ds_filename);
                // move file to metadata subdirectory
                rename($ds_filename, $tmp_ds_directory . '/metadata/' . $basename);
                // Add the file to $files_to_add array
                $files_to_add[] = array(
                    'source' => $tmp_ds_directory . '/metadata/' . $basename,
                    'dest' => 'metadata/' . basename($ds_filename),
                );
            }
        }
    }
    if (! $objects) {
        // parents in the compound object model do not have an OBJ, AM doesn't like
        $parent_file_path = $tmp_ds_directory . DIRECTORY_SEPARATOR . 'parent.txt';
        file_put_contents($parent_file_path, $islandora_object->id .
            " is the parent in a compound object. OBJ files are in the children.\n");
        $files_to_add[] = array(
            'source' => $parent_file_path,
            'dest' => 'objects/OBJ.txt',
        );
    }

    if (module_exists('islandora_premis')) {
        module_load_include('inc', 'islandora_premis', 'includes/utilities');
        $premis_xml = islandora_premis_transform_foxml_to_premis($islandora_object);

        $premis_file_path = $tmp_ds_directory . DIRECTORY_SEPARATOR . 'premis.xml';
        file_put_contents($premis_file_path, $premis_xml);
        $files_to_add[] = array(
            'source' => $premis_file_path,
            'dest' => 'metadata/PREMIS.xml',
        );
    }

    if (count($files_to_add)) {
        return $files_to_add;
    }
    else {
        return FALSE;
    }
}
