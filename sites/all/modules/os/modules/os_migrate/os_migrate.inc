<?php 

/**
 * @class OSMigration
 *
 * Abstract class for all OpenScholar migration classes
 * Sets some defaults
 * Provides d6_db_select helper for getting d6 db for import
 */
abstract class OSMigration extends Migration {
  public function __construct() {
    // Always call the parent constructor first for basic setup
    parent::__construct();

    // With migrate_ui enabled, migration pages will indicate people involved in
    // the particular migration, with their role and contact info. We default the
    // list in the shared class; it can be overridden for specific migrations.
    $this->team = array(
      new MigrateTeamMember('Jon Sagotsky', 'jsagotsky@fas.harvard.edu', t('Migration Lead')),
    );

    // Individual mappings in a migration can be linked to a ticket or issue
    // in an external tracking system. Define the URL pattern here in the shared
    // class with ':id:' representing the position of the issue number, then add
    // ->issueNumber(1234) to a mapping.
    $this->issuePattern = 'https://redmine.hmdc.harvard.edu/issues/:id:';
  }
  
  /**
   * @function d6_db_select
   * 
   * As db_select, but targets old d6 database
   * Sends users to admin page on a misconfigured db
   */
  public function d6_db_select($target, $alias = NULL, array $options = array()) {
    static $os_migrate_database;  
    if (!isset($os_migrate_database)) {
      $os_migrate_database =  variable_get('os_migrate_database', '');
    }
  
    try {
      return Database::getConnection('default', $os_migrate_database) 
        ->select($target, $alias, $options);
    } catch (DatabaseConnectionNotDefinedException $e) {
      flood_register_event('os_migrate_bad_db');
      $msg = t('Unable to connect to db %db.  Please !link', array(
      				  '%db' => $os_migrate_database, 
      				  '!link'=>l('check settings','admin/config/os_migrate')));
      drupal_set_message($msg, 'error');
                  
      throw $e;
    }
  }
  
}