<?php

/**
 * @file
 * 
 * Abstract migration class for nodes
 **/


/**
 * Abstract node migration class
 * 
 * Defines mappings for node table.  Let each type implement this and dscribe the specifics
 */
abstract class AbstractNodeOSMigration extends OSMigration {

  public function __construct($destination_bundle, $source_content_type, $create = TRUE) {
    $this->bundle = $destination_bundle;
    
    parent::__construct(MigrateGroup::getInstance('node', array('inline', 'users')));    
    //when extending this class, don't forget to update info file with new filename!

    // if desired, supply node with defaults.  
    if ($create) {   
      $this->source = $this->source($source_content_type); //condtion type?
      $this->destination = $this->destination($destination_bundle); 
      $this->map = $this->map($source_content_type);
      $this->dependencies = array('UsersOS', 'EmbedInlineOS', 'ImageInlineOS', 'FileInlineOS', 'HTMLInlineOS'); //og when that's done
      
      /* mappings */
      $this->addSimpleMappings(array('created','changed','status','promote','sticky','comment',
                                     'title', 'tnid', 'uid'));
      $this->addFieldMapping('language', 'language')->defaultValue('und');
      $this->addFieldMapping('nid', 'nid');
      $this->addFieldMapping('is_new')->defaultValue(TRUE);
      $this->addFieldMapping('revision_uid', 'nr_uid');
      $this->addFieldMapping('body', 'body')
        ->arguments(array('format' => array('source_field'=>'format')));
        //>arguments(array('format'=>'filtered_html'));
      $this->addFieldMapping('log', 'log'); //from nr
      $this->addFieldMapping('revision')->defaultValue(TRUE); //create a new revision.  vid is automatically migrated
      $this->addFieldMapping('path', 'path');
      $this->addFieldMapping('pathauto')->defaultValue(FALSE); //do not create new paths.  paths already exist.
      
      parent::addMappingsCCK(); //map additional cck fields
      
      
      $associated_files_args = MigrateFileFieldHandler::arguments(NULL, 'file_copy', FILE_EXISTS_REPLACE);
      $this->addFieldMapping('field_file_upload', 'file_upload')->arguments($associated_files_args);
      
      /* Unmapped source fields.  Mark them as Do Not Map, so the UI knows they're intentionally ignored */
      $this->addUnmigratedSources(array('vid', 'moderate', 'translate', 'type', 'name'), t('DNM'));
//      $this->addUnmigratedSources(array('oe_vid', 'oe_nid', 'delta', 'field_os_inline_oembed_url', 'field_os_inline_oembed_title', 'field_os_inline_oembed_attributes') ,t('DNM'));
      //moderate appears to have been removed from node.
      //type.  we're doing nodes one type at a time.  type should be handled there.
      //translate unknown
      
    }
    //else - user is responsible for doing parent:query() and setting up source, destination, and map
  }
  
  /**
   * several helper functions for setting up child nodes.  
   * it's not clear to me yet what level of abstraction I'll be working at
   * so for now I'm leaivng it up to the node that implements this class 
   * to use the provided options or come up with their own.
   */
  
  public function query($content_type) {
    $query = parent::d6_db_select('node', 'n')
                    ->fields('n', $this->getNodeFields())
                    ->condition('n.type', $content_type, '=');
                    
    //nodes to test files with:
    //$query->condition('n.nid', '20301', '='); //prod node with uploads
    //$query->condition('n.nid', '6811', '='); //prod node with inline files
    //$query->condition('n.nid', '13701', '='); //prod node with multiple images
    
    $query->join('node_revisions', 'nr', 'n.vid = nr.vid');
    $query->fields('nr', array('body', 'uid', 'log'));
    
    $query->join('filter_formats', 'ff', 'nr.format = ff.format');
    $query->addField('ff', 'name', 'format_name');
    $query->orderBy('n.changed');
               

    parent::addQueryCCK($query, $content_type); //include joins for cck fields base on class's cck_fields array
    
    return $query;
  }
  
  public function sourceFields() {
    $source_fields =  array(
    	'nid'   => t('Node ID'),
      'path'  => t('URL Path Alias'),
      'file_upload' => t('File attachments'), //this may have to move to individual classes
      //'nr.body'	=> t('Node\'s Content')
    );
    
    $source_fields+= parent::sourceFieldsCCK();
    return $source_fields;
  }
  
  public function source($content_type) {
    return new MigrateSourceSQL($this->query($content_type), $this->sourceFields(), null, array('map_joinable' => FALSE));
  }
  
  public function destination($bundle) {
    return new MigrateDestinationNode($bundle);
  }
  
  //map provides the schema for the primary key of the source table.  this lets migrate associate a unique source id
  //with a migrated object
  public function map($content_type) {
    return new MigrateSQLMap($this->machineName,
      array(
      	'nid' => array(
          'type' => 'int',
          'not null' => TRUE,
          'unsigned' => TRUE,
          'description' => 'D6 ' . $content_type . ' ID.',
          'alias' => 'n',
        ),
      ),
      MigrateDestinationNode::getKeySchema()
    );
  }
  /**
   * @function getNodeFields()
   *
   *	helper function returns array of fields in node
   **/  
  function getNodeFields() {
    return array('nid', 'vid', 'type','language','title','uid','status','created',
                 'changed','comment','promote','moderate','sticky','tnid','translate',
    );
  }
  



  /**
   * Implements prepareRow
   * 
   * Fetches path from {url_alias}
   */
  function prepareRow($current_row) {
    
    //check if bundle exists.
    $info = field_info_bundles('node');
    if (!isset($info[$this->bundle])) {
      Migration::displayMessage(t('Could not migrate into non-existant bundle'));
      return FALSE;
    }
    
    //get path from {url_alias}
    $query = parent::d6_db_select('url_alias', 'ua')
      ->fields('ua', array('dst'))
      ->condition('ua.src', 'node/'.$current_row->nid, '=');
    $results = $query->execute()->fetchAssoc();
    $current_row->path = $results['dst'];
    
    //get file uploads
    $current_row->file_upload = parent::os_migrate_get_file_uploads($current_row->vid);
    // file upload fields aren't in current page type.  
    
    //lanuage should be explicitly undefined instead of blank
    if (!strlen($current_row->language)) {
      $current_row->language = 'und';
    }
    
    //format.  legacy formats are Filtered HTML, Full HTML, PHP Code, Comments HTML, Minimal WYSIWYG, FUll WYSWIWYG
    switch ($current_row->format_name) {
      case 'PHP Code':
        $current_row->format = 'plain_text';
        break;
      default:
        $current_row->format = 'filtered_html';
    }
    
    //adjust the body to use new fids for inline objects
    parent::os_migrate_inline_update($current_row);
    
    //prep cck field data
    parent::prepareRowCCK($current_row);
    
  }
  
}