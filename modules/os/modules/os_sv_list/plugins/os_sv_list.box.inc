<?php
class os_sv_list_box extends os_boxes_default {

  /**
   * Implementation of boxes_content::options_defaults().
   */
  public function options_defaults() {
    $options = parent::options_defaults();
    $biblio_types = $this->_biblio_types();
    $biblio_whitelist = array_combine(array_keys($biblio_types), array_keys($biblio_types));

    $options += array(
      'content_type' => 'all',
      'display' => 'display_title',
      'more' => FALSE,
      'more_text' => t('More'),
      'more_link' => '', // what's a good default?

      'pager' => FALSE,
      'number_of_items' => 6,
      'sort_by' => 'sort_newest',
      'vocabs' => array(),
      'terms' => array(),

      //Do not include subsites by default
      'query_scope' => array('current'),

      'biblio_whitelist' => $biblio_whitelist,
    );

    return $options;
  }

  /**
   * Implementation of boxes_content::options_form().
   */
  public function options_form(&$form_state) {
    $form = parent::options_form($form_state);
    $defaults = $this->options_defaults();
    drupal_add_js(array('os_sv_list_box' => array('new_box' => $this->new)), 'setting');

    $content_types = array(
      'all' => t('All')
    ) + os_get_bundles(array(1));

    if (isset($content_types['media_gallery'])) {
      $content_types['image'] = t('Images');
    }

    $form['#submit'] = array('os_sv_list_form_submit');

    $content_types_value = !empty($form_state['values']['content_type']) ? $form_state['values']['content_type'] : $this->options['content_type'];

    $form['content_type'] = array(
      '#type' => 'select',
      '#title' => t('Content Type'),
      '#default_value' => $content_types_value,
      '#options' => $content_types,
      '#id' => 'os_sv_list_content_type',
      '#attached' => array(
        'js' => array(
          drupal_get_path('module', 'os_sv_list').'/os_sv_list.js',
        ),
      ),
      '#ajax' => array(
        'callback' => 'os_sv_list_vocabs_content_type',
        'wrapper' => 'query-details',
      ),
    );

    $form['biblio_whitelist'] = array(
      '#type' => 'checkboxes',
      '#title' => t('Select publication types to display'),
      '#default_value' => isset($this->options['biblio_whitelist']) ? $this->options['biblio_whitelist'] : $defaults['biblio_whitelist'],
      '#options' => $this->_biblio_types(),
      '#prefix' => '<div id="os_sv_list_biblio_whitelist">',
      '#suffix' => '</div>',
      '#attached' => array(
        'js' => array(
          drupal_get_path('module', 'os_boxes').'/misc/selectAll.js',
        ),
      ),
      '#states' => array(
        'visible' => array(
          '#os_sv_list_content_type' => array('value' => 'biblio'),
        ),
        'invisible' => array(
          '#os_sv_list_content_type' => array('!value' => 'biblio')
        ,)
      )
    );

    if(module_exists('vsite')){

      ctools_include('subsite','vsite');

      $form['query_scope'] = array(
        '#title' => 'Include results from',
        '#type' => 'checkboxes',
        '#options' => array('current' => "Current Site", 'subsites' => 'Subsites'),
        '#default_value' => $this->options['query_scope'],
        '#access' => vsite_has_subsites(),
        '#required' => TRUE,
      );
    }

    $form['display'] = array(
      '#type' => 'select',
      '#title' => 'Display style',
      '#default_value' => $this->options['display'],
      '#options' => array(
        'display_title' => t('Title'),
        'display_sidebar_teaser' => t('Sidebar Teaser'),
        'display_teaser' => t('Teaser'),
        'display_body' => t('Entire Post'),
        'display_links' => t('Link'),
      ),
      // This #states rule limits use to when the $form['query_scope']
      // 'subsites' checkbox is not checked."
      '#states' => array(
        'visible' => array(// action to take.
          ':input[name="query_scope[subsites]"]' => array('checked' => FALSE),
        ),
      ),
    );
    //defines which display types are content type specific
    $form['display']['#attached']['js'][] = array('type'=>'setting', 'data'=>array('os_sv_list' => array('has_display' => array(
      'display_sidebar_teaser' => array('news', 'event', 'person'),
      'display_links' => array('link'),
    ))));

    $form['sort_by'] = array(
      '#type' => 'select',
      '#title' => 'Sorting',
      '#default_value' => $this->options['sort_by'],
      '#options' => array(
        //content type sensitive sorts
        'sort_event' => t('Event Date'),
        'sort_presentation' => t('Recently Presented'),
        'sort_biblio' => t('Year of Publication'),
        'sort_news' => t('News Date'),

        //standard sorts
        'sort_newest' => t('Newest Post'),
        'sort_oldest' => t('Oldest Post'),
        'sort_alpha' => t('Alphabetical'),
      )
    );

    $form['query_details'] = array(
      '#prefix' => '<div id="query-details">',
      '#suffix' => '</div>',
      '#parents' => array('query_details'),
    );

    if ($vocabs = os_sv_list_get_content_type_vocabs($content_types_value)) {
      // Make the vocabulary selection optional.
      $vocabs = array(-1 => t('- Select -')) + $vocabs;
    }
    $vocabs_default_value = !empty($form_state['values']['vocabs']) ? $form_state['values']['vocabs'] : $this->options['vocabs'];
    $terms_default_value = !empty($form_state['values']['terms']) ? $form_state['values']['terms'] : $this->options['terms'];

    $form['query_details']['vocabs'] = array(
      '#type' => 'select',
      '#title' => t('Vocabulary'),
      '#description' => t('Select a vocabulary to filter content by tagged items.'),
      '#options' => $vocabs,
      '#default_value' => $vocabs_default_value,
      '#access' => $vocabs,
      '#ajax' => array(
        'callback' => 'os_sv_list_terms_details_ajax',
        'wrapper' => 'terms-details',
      ),
    );

    $terms = array();
    if ($vocabs_default_value) {
      $tree = taxonomy_get_tree($vocabs_default_value);
      foreach ($tree as $item) {
        $term = taxonomy_term_load($item->tid);
        $terms[$term->tid] = $term->name;
      }
    }

    $form['query_details']['terms'] = array(
      '#type' => 'checkboxes',
      '#title' => t('Vocabulary terms'),
      '#description' => empty($terms) ? t('No terms in the vocabulary') : '',
      '#options' => $terms,
      '#access' => $vocabs,
      '#states' => array(
        'invisible' => array(
          ':input[name="vocabs"]' => array('value' => -1),
        ),
      ),
      '#default_value' => $terms_default_value,
      '#prefix' => '<div id="terms-details">',
      '#suffix' => '</div>',
    );

    $terms_statement = isset($form_state['values']['content_type']) && $form_state['values']['content_type'] == -1;
    $vocabs_statement = isset($form_state['values']['vocabs']) && $form_state['values']['vocabs'] == -1;

    // Hide the form but keep the prefix and suffix for the AJAX callback.
    if ($terms_statement || $vocabs_statement) {
      $form['query_details']['terms'] = array(
        '#prefix' => '<div id="terms-details">',
        '#suffix' => '</div>',
      );
    }

    $form['query_details']['more'] = array(
      '#title' => 'Show "more" link',
      '#type' => 'checkbox',
      '#default_value' => $this->options['more'],
    );

    //populate list of content types => paths
    ctools_include('os', 'os');
    $content_map = os_get_app_path();
    $content_map = array_diff_key($content_map, array_flip(array('bio', 'cv', 'class_material')));
    if (isset($content_types['image'])) {
      $content_map['image'] = $content_map['media_gallery'];
    }
    $content_map += array_fill_keys(array_keys(array_diff_key($content_types, $content_map)),' '); //use blank path for unknown paths.


    if (!empty($content_map[$content_types_value]) && (empty($this->options['more_link']) || in_array($this->options['more_link'], $content_map))) {
      $default_value = $content_map[$content_types_value];
    }
    else {
      $default_value = $this->options['more_link'];
    }

    if (!empty($form_state['values']['more_link'])) {
      // Unset the values from the text field, as otherwise FAPI will add
      // it to the #value of the form-element.
      // @see http://drupal.org/node/1024962#comment-4021826
      unset($form_state['input']['more_link']);
    }

    $form['query_details']['more_link'] = array(
      '#title' => t('URL for the more link'),
      '#type' => 'textfield',
      '#default_value' => $default_value,
      '#states' => array(
        'visible' => array('input[name=more]' => array('checked' => TRUE)),
        'invisible'  => array('input[name=more]' => array('checked' => FALSE)),
      ),
      '#prefix' => '<div id="more_link_div">',
      '#suffix' => '</div>',
      '#attached' => array(
        'js' => array(
          array(
            'data' => array(
              'more_link_defaults' => $content_map,
            ),
            'type' => 'setting',
          )
        )
      )
    );

    $form['query_details']['more_text'] = array(
      '#title' => 'Label of the more link (default: "more")',
      '#type' => 'textfield',
      '#default_value' => $this->options['more_text'],
      '#states' => array(
        'visible' => array('input[name=more]' => array('checked' => TRUE))
      )
    );

    $form['pager'] = array(
      '#title' => 'Show pager',
      '#type' => 'checkbox',
      '#default_value' => $this->options['pager']
    );

    if(module_exists('vsite')){

      ctools_include('subsite','vsite');

	    $form['query_scope'] = array(
	      '#title' => 'Include results from',
	      '#type' => 'checkboxes',
	      '#options' => array('current' => "Current Site", 'subsites' => 'Subsites'),
	      '#default_value' => $this->options['query_scope'],
	      '#access' => vsite_has_subsites(),
	      '#required' => TRUE,
	    );
    }

    $form['number_of_items'] = array(
      '#title' => 'Number of items to display',
      '#type' => 'select',
      '#options' => array_slice(range(0, 20), 1, NULL, TRUE),
      '#default_value' => $this->options['number_of_items']
    );

    return $form;
  }
  /**
   * // * Implementation of boxes_content::render().
   */
  public function render() {
    $block = parent::render();
    $options = $this->options;
    $page_num = isset($_GET['page']) ? $_GET['page'] : 0;
    $page = 0;


    $data = $this->get_nids($page_num, $page);
    $more_link = FALSE;

    if (count($data)) {
      if (count($data) >= $options['number_of_items']) {
        $data = array_slice($data, 0, $options['number_of_items']);
        $more_link = TRUE;
      }

      $display_func = (method_exists($this, $options['display'])) ? $options['display'] : 'display_title';

      if ($options['content_type'] == 'image') {
        $display_func = 'display_image';
        $nodes = file_load_multiple($data);
      }else{
        $nids = array();
        $extra = array();
        foreach ($data as $n) {
          $n = explode('|', $n);
          $nids[] = $n[0];
          if (isset($n[1])) {
            $extra[] = explode('=', $n[1]);
          }
        }
        if(count($extra)){
	        foreach ($nids as $delta => $n) {
	          $node = clone node_load($n);
	          if(isset($extra[$delta])){
	            $node->{$extra[$delta][0]} = $extra[$delta][1];
	          }
	          $nodes[] = $node;
	        }
        }else{
          $nodes = node_load_multiple($nids);
        }
      }

      $block['content'] = $this->$display_func($nodes);
    }

    // pager widget
    $pager = '';
    if ($options['pager'] && $block['content']) {
      // theme_pager();
      $parameters = array(
        'sv_list_box_delta' => $this->delta,
        'pager_id' => $page,
      );
      $pager = theme('views_mini_pager', array(
        'parameters' => $parameters,
        'element' => $page,
        'quantity' => $options['number_of_items'],
      ));
    }

    // more link
    if ($options['more'] && $more_link && $block['content']) {
      $block['content'] .= l($options['more_text'], $options['more_link'], array('attributes' => array('class' => array('more-link'))));
    }

    drupal_add_library('system', 'drupal.ajax');
    drupal_add_js(drupal_get_path('module', 'os_sv_list'). '/os_sv_list_display.js');

    if ($block['content']) {
      $block['content'] = '<div id="box-' . $this->delta . '-page" data-page="'.$page_num.'" data-delta="'.$this->delta.'">' . $block['content'] . '</div>' . $pager;
      return $block;
    } else {
      $block['title'] = $block['subject'] = '';
      return $block;
    }

  }

  // get_default_query_query - humble beginnings of the EFQ
  function get_default_query() {
    //Only filter to public bundles if we are not including subsites
    if (!isset($this->options['query_scope']) || !in_array('subsites', array_filter($this->options['query_scope']))){
      $bundles = os_get_bundles(array(1));
      if (empty($bundles)) {
        return FALSE;
      }
    }

    // defaults
    ctools_include('node', 'os');
    $efq = new OSNodeFieldQuery(array_keys($bundles),FALSE);
    return $efq;
  }

   /**
   * Queries the DB for all nids in the box
   *
   * @param int $page_num
   *   Specifies a page.  FALSE to get all nids.
   * @return multitype:
   */
  public function get_nids($page_num = FALSE, &$page = NULL) {

    $options = $this->options;
    $entity_field = "n.nid";

    //Build the Query, EFQ or otherwise.
    switch ($options['content_type']) {
      case 'biblio':
        $query = db_select('node', 'n')
          ->condition('n.type', 'biblio')
          ->condition('n.status', 1);
        $query->join('biblio', 'b', 'n.vid = b.vid');

        //check biblio show all...
        if (isset($options['biblio_whitelist']) && (count(array_filter($options['biblio_whitelist'])))) {
          $bib_types = array_filter($options['biblio_whitelist']);
          $query->condition('biblio_type', array_values($bib_types), 'IN');
        }

        switch($options['sort_by']) {
          case 'sort_biblio': $query->orderBy('biblio_year', 'DESC'); $query->orderBy('n.created', 'DESC'); break;
          case 'sort_newest': $query->orderBy('n.created', 'DESC'); break;
          case 'sort_oldest': $query->orderBy('n.created', 'ASC'); break;
          case 'sort_alpha':  $query->orderBy('n.title', 'ASC'); break;
        }

        $query->fields('n', array('nid'));
        break;

      case 'image':
        $query = new EntityFieldQuery();
        $query->entityCondition('entity_type', 'file');
        // Add image as a condition, this will remove all media from showing up.
        $query->propertyCondition('type', 'image');
        $sorts = array(
          'sort_newest' => array('timestamp', 'DESC'),
          'sort_oldest' => array('timestamp', 'ASC'),
          'sort_alpha' => array('filename', 'ASC'),
        );
        $sort = (isset($sorts[$options['sort_by']])) ? $sorts[$options['sort_by']] : $sorts['sort_newest'];
        $query->propertyOrderBy($sort[0], $sort[1]);

      break;

      case 'event':
        //Only query the date field if they are sorting by it
        if($options['sort_by'] == 'sort_event'){
          $query = db_select('field_data_field_date', 'fd')
          ->fields('fd', array('entity_id', 'delta'))
          ->where('field_date_value > NOW()')
          ->orderBy('field_date_value');
          $entity_field = "fd.entity_id";
          break;
        }
      default:
        if (($query = $this->get_default_query()) == FALSE) {
          return array();
        }

        // content type
        if ($options['content_type'] != 'all') {
          $query->propertyCondition('type', $options['content_type']);
        }
        break;
    }

    //Filter based on site or subsite
    $gids = array();
    if(module_exists('vsite')){
      if(!isset($options['query_scope']) || !count(array_filter($this->options['query_scope']))){
        $options['query_scope'] = array('current');
      }

	    foreach (array_filter($options['query_scope']) as $scope){
	      switch ($scope){
	      	case 'current':
	      	  $vsite = spaces_get_space();
	      	  if (is_object($vsite) && is_numeric($vsite->id)) {
	      	    $gids[$vsite->id] = $vsite->id;
	      	  }
	      	  break;
	      	case 'subsites':
	      	  ctools_include('subsite','vsite');
	      	  $gids = array_merge($gids,vsite_get_subsites());
	      	  break;
	      }
	    }

	    $operator = (count($gids) == 1)? "=" : "IN";
	    $gids = (count($gids) == 1)? current($gids) : $gids;

	    if($query instanceof EntityFieldQuery){
	      $query->fieldCondition(OG_AUDIENCE_FIELD, 'target_id', $gids, $operator);
	    }else{
	      $query->join('og_membership', 'og', "og.etid = {$entity_field}");
	      $query->condition('og.gid', $gids, $operator);
	      $query->condition('og.entity_type', 'node');
	      $query->condition('og.group_type', 'node');
	    }
    }

    //Filter Exceptions.
    switch ($options['content_type']) {
      case 'book':
        //limit book depth to 1.  not a field, so use a regular query
        $book_query = db_select('menu_links', 'ml')
          ->condition('module', 'book')
          ->condition('depth', 1);
        $book_query->addField('ml', 'link_path');
        $book_nids = $book_query->execute()->fetchCol();
        $book_nids = preg_replace('/node\//', '', $book_nids);
        $book_nids = array_values($book_nids);

        if (count($book_nids)) {
          $query->propertyCondition('nid', $book_nids, 'IN');
        }
        break;
    }

    //Init Pager.
    if ($options['pager'] && is_int($page)) {
      if($query instanceof EntityFieldQuery){
        $query->pager($options['number_of_items']);
        $page = $query->pager['element'];
      }else{
        $query = $query->extend('PagerDefault');
        $query->limit($options['number_of_items']);
        $page = PagerDefault::$maxElement++;
        $query->element($page);
      }
    }else if (is_int($page)){
      $query->range(0, $this->options['number_of_items']);
    }


    //sorting exceptions
    switch ($options['content_type']) {
      case 'person':
        //person lists sorting alphabetically should use last name, not first.
        if ($this->options['sort_by'] == 'sort_alpha') {
          $query->fieldOrderBy('field_last_name', 'value', 'ASC');
        }
      break;
    }

    // Sort the results.
    if ($query instanceof EntityFieldQuery && $options['content_type'] != 'image') {
      $sort_func = (method_exists($this, $options['sort_by'])) ? $options['sort_by'] : 'sort_newest';
      $this->$sort_func($query);
    }

    //For all Queries but image, filter out based on vocab.
    if ($options['content_type'] != 'image' && !empty($this->options['vocabs']) && ($this->options['vocabs'] != -1)) {
      $tagged_nodes = $this->tagged_nodes();

      // If a vocabulary was selected and there were no tagged nodes then
      // we don't need to return any node.
      $operator = (empty($tagged_nodes))? "=" : "IN";
	    $tagged_nodes = (empty($tagged_nodes))? 0 : $tagged_nodes;

	    if($query instanceof EntityFieldQuery){
        $query->propertyCondition('nid', $tagged_nodes, $operator);
      }else{
        $query->condition($entity_field, $tagged_nodes, $operator);
      }
    }

    // Execute the Query.
    switch ($options['content_type']) {
      case 'biblio':
        $nids = $query->execute()->fetchCol();
        break;

      case 'image':
        $result = $query->execute();
        $nids = isset($result['file']) ? array_keys($result['file']) : array();
        break;
      case 'event':
        //Only get the date value if they are sorting by it
        if($options['sort_by'] == 'sort_event'){
          $query = $query->execute();

          foreach ($query as $r) {
            $nids[] = $r->entity_id.'|date_id=date.'.$r->entity_id.'.field_date.'.$r->delta;
          }
          break;
        }
      default:

        $result = $query->execute();
        $nids = isset($result['node']) ? array_keys($result['node']) : array();
        break;
    }

    return $nids;
  }

  /**
   * Return the nodes attached to terms of the selected vocabulary.
   */
  public function tagged_nodes() {
    if (empty($this->options['vocabs'])) {
      return;
    }

    // Load the taxonomy terms of the vocabulary.
    if (!$vocabulary = taxonomy_get_tree($this->options['vocabs'])) {
      return;
    }

    // Clean terms with empty value.
    if (!empty($this->options['terms'])) {
      foreach ($this->options['terms'] as $key => $value) {
        if ($value == 0) {
          unset($this->options['terms'][$key]);
        }
      }
    }

    // Collect all the taxonomy term ID.
    if (!empty($this->options['terms'])) {
      $tids = $this->options['terms'];
    }
    else {
      $tids = array();
      foreach ($vocabulary as $terms) {
        $tids[] = $terms->tid;
      }
    }

    $bundles = os_get_bundles(array(1));
    $content_types = ($this->options['content_type'] == 'all') ? array_keys($bundles) : $this->options['content_type'];

    $query = db_select('taxonomy_index', 't');
    $query->join('node', 'n', 'n.nid = t.nid');
    $result = $query
      ->condition('t.tid', $tids, 'IN')
      ->condition('n.type', $content_types)
      ->fields('t')
      ->execute();

    $nids = array();
    foreach ($result as $row) {
      $nids[] = $row->nid;
    }

    return $nids;
  }

  /**
   * sorting functions
   */
  function sort_newest(&$efq) {
    $efq->propertyOrderBy('created', 'DESC');
  }
  function sort_oldest(&$efq) {
    $efq->propertyOrderBy('created', 'ASC');
  }
  function sort_alpha(&$efq) {
    $efq->propertyOrderBy('title', 'ASC');
  }

  function sort_news(&$efq) {
    $efq->fieldOrderBy('field_news_date', 'value', 'DESC');
  }

  function sort_presentation(&$efq) {
    $efq->fieldOrderBy('field_presentation_date', 'value', 'DESC');
    $this->sort_newest($efq);
  }

  /**
   * Display functions
   */
  function display_view_mode($nodes, $viewmode = 'full') {
    $output = array();
    foreach ($nodes as $n) {
      $output[] = node_view($n, $viewmode);
    }

    return drupal_render($output);
  }

  function display_body($nodes) {
    return $this->display_view_mode($nodes, 'full');
  }

  function display_sidebar_teaser($nodes) {
    return $this->display_view_mode($nodes, 'sidebar_teaser');
  }

  function display_teaser($nodes) {
    return $this->display_view_mode($nodes, 'teaser');
  }

  function display_title($nodes) {

    $cross_site_links = FALSE;
    if(module_exists('vsite') && isset($this->options['query_scope']) && in_array('subsites', array_filter($this->options['query_scope']))){
      ctools_include('subsite','vsite');
      $cross_site_links = TRUE;
    }

    foreach ( $nodes as $node ) {
      $options = array();
      if($cross_site_links){
        vsite_subsite_node_url_options($node, $options);
      }

      $items[] = l($node->title, 'node/' . $node->nid, $options);
    }

    return theme('item_list', array(
      'items' => $items,
      'type' => 'ul'
    ));
  }

  function display_links($nodes) {
    $links = array();
    foreach ($nodes as $node) {
      $field_link = field_get_items('node', $node, 'field_links_link');
      if(count($field_link) && strlen($field_link[0]['url'])){
        $links[] = l($node->title, $field_link[0]['url']);
      }
    }

    return theme('item_list', array(
      'items' => $links,
      'type' => 'ul',
      'attributes' => array(
        'class' => 'link-list',
      ),
    ));
  }

  function display_image($files) {
    //$html = '';
    $items = array();

    foreach ($files as $image) {
      $variables = array(
        'style_name' => "os_square_50_50",
        'path' => $image->uri,
        'width' => 50,
        'height' => 50,
      );
      //$html .= theme('image', $variables);
      $img = theme_image_style($variables);
      $items[] = l($img, file_create_url($image->uri), array('html'=>TRUE, 'attributes' => array('rel' => 'recent-images', 'class' => 'colorbox'))); //get a real url, hippie!
    }

    return theme('item_list', array('items'=>$items, 'attributes'=>array('class'=>array('recent-images-list'))));
  }

  function _biblio_types() {
    $rows = db_select('biblio_types', 'bt')
      ->fields('bt', array('tid', 'name'))
      ->condition('tid', 0, '>')
      ->condition('visible', 1)
      ->execute();
    $options = array('all' => 'Select All');
    foreach ($rows as $r) {
      $options[$r->tid] = $r->name;
    }

    return $options;
  }
}

/**
 * AJAX callback to add query details (e.g. the vocabulary to use).
 *
 * Since the controlling logic for populating the form is in the form builder
 * function, all we do here is select the element and return it to be updated.
 */
function os_sv_list_query_details_ajax($form, &$form_state) {
  return $form['options']['query_details'];
}

/**
 * AJAX callback for adding terms to the query.
 *
 * Since the controlling logic for populating the form is in the form builder
 * function, all we do here is select the element and return it to be updated.
 */
function os_sv_list_terms_details_ajax($form, &$form_state) {
  return $form['options']['query_details']['terms'];
}

/**
 * AJAX callback for reload the vocabulary for the selected content type.
 */
function os_sv_list_vocabs_content_type($form, $form_state) {
  return $form['options']['query_details'];
}
