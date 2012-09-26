<?php

/**
 * @file
 * Implements TableCopyMigration for several direct table copies
 */

class IQSSPinserverRegisterTableMigration extends TableCopyMigration {

  public function __construct() {
    $this->required_modules = array('iqss_pinserver_register');
    $key_schema = array(
        'uid' => array(
            'type' => 'int',
            'unsigned' => TRUE,
            'not null' => TRUE,
        ),
    );

    $table = 'iqss_pinserver_register';
    parent::__construct($table, $key_schema);
  }
}


class CitationDistributeTableMigration extends TableCopyMigration {
  public function __construct() {
    $this->required_modules = array('citation_distribute');

    $key_schema = array(
      'nid' => array(
        'type' => 'int',
        'unsigned' => TRUE,
        'not null' => TRUE,
      ),
      'plugin' => array(
        'type' => 'varchar',
        'length' => 64,
        'not null' => TRUE,
      ),
    );

    $table = 'citation_distribute';

    parent::__construct($table, $key_schema);
  }
}

class VariableTableMigration extends TableCopyMigration {

  public function __construct() {
    $this->whitelist = array('mollom_private_key', 'mollom_public_key', 'os_mailchimp_api', 'os_mailchimp_list', 'olark_code',
      'apachesolr_host', 'apachesolr_path', 'apachesolr_port', 'iqss_rbuild_default_url', 'purl_base_domain', 'site_mail',
      'site_mission', 'site_name', 'site_offline_message',
    );

    $this->whitelist = array_merge($this->whitelist, parent::subfields('citation_distribute', array('dash_password', 'module_mode', 'service_googlescholar_auto_flag', 'service_repec_archive_code', 'service_repec_inst_name', 'service_repec_maintainer_email', 'service_repec_maintainer_name', 'service_repec_provider_homepage', 'service_repec_provider_institution', 'service_repec_title', 'service_repec_archive_path', 'service_repec_archive_url'), '_'));
    $this->whitelist = array_merge($this->whitelist, parent::subfields('iqss_gking', array('alternate_theme_nodes', 'heading_disabled_terms', 'override_child_terms'), '::'));
    $this->whitelist = array_merge($this->whitelist, parent::subfields('pinserver', array('app_name', 'gpg_dir', 'error_log', 'landing_path', 'pin_url'), '_'));
    // the following look like they should be migrated, but I'm not sure
    //pinserver*, shorten*,

    $key_schema = array(
      'name' => array(
        'type' => 'varchar',
        'length' => 128,
        'not null' => TRUE,
      ),
    );

    $table = 'variable';
    parent::__construct($table, $key_schema);
  }

  public function query() {
    $query = parent::query();
    $query->condition('name', $this->whitelist, 'IN');
    return $query;
  }

  //clear the menu cache after import.
  public function postImport() {
    parent::postImport();
    drupal_flush_all_caches();
  }
}

class UrlAliasTableMigration extends TableCopyMigration {

  public function __construct() {
    $key_schema = array(
      'pid' => array(
        'type' => 'int',
        'unsigned' => TRUE,
        'not null' => TRUE,
      ),
    );

    $table = 'url_alias';
    parent::__construct($table, $key_schema);

    //stash pids here so we don't overwrite any imported with nodes/users
    $this->used_pids = db_select('url_alias')->fields('url_alias', array('pid'))->execute()->fetchCol();
  }

  public function query() {
    $query = parent::query();

    $prefixes = array('nodeorder', 'iqss_gking_research_group', 'filefield_paths'); //taxonomies too?
    $or = db_or();
    foreach($prefixes as $prefix) {
      $or->condition('src', $prefix.'/%', 'LIKE');
    }

    $query->condition($or);

    return $query;
  }

  public function prepareRow(&$row) {
    $row->source = $row->src;
    unset($row->src);

    $row->alias = $row->dst;
    unset($row->dst);

    $row->pid = $this->getPID($row->pid);
  }

  //returns this pid if available.  if not, gets new one.  updates list
  function getPID($pid) {
    if (!in_array($pid, $this->used_pids)) {
      return $pid;
    }

    $new = 1 + max($this->used_pids);
    $this->used_pids[] = $new;
    return $new;
  }
}

class UsageLogTableMigration extends TableCopyMigration {
  public function __construct() {
    $this->required_modules = array('openscholar_log_usage');

    $key_schema = array(
      'host' => array(
        'type' => 'varchar',
        'length' => 255,
        'not null' => TRUE,
      ),
      'vsite' => array(
        'type' => 'varchar',
        'length' => 100,
        'not null' => TRUE,
      ),
      'age' => array(
        'type' => 'varchar',  //used to be datetime.
        'length' => 100,
        'not null' => TRUE,
      )
    );

    $table = 'openscholar_usage_log'; //why is the table named differently than the module?

    parent::__construct($table, $key_schema);
  }

  public function query() {
    //check if old table exists.
    if (parent::d6_db_table_exists('openscholar_usage_log')) {
      //return actual query
      return parent::query();
    } else {
      //return something null that won't give errors when called.
      return parent::d6_db_select('node', 'n')
        ->isNull('nid');

    }
  }

}
