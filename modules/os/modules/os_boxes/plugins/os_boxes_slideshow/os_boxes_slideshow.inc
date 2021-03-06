<?php

/**
 * @file
 * The box class for "Twitter feed" type widgets.
 */

class os_boxes_slideshow extends os_boxes_default {

  /**
   * Implementation of boxes_content::options_defaults().
   */
  public function options_defaults() {
    $options = parent::options_defaults();
    $options['gallery_nid'] = 'all';
    $options['pager'] = 'none';
    $options['controls'] = 'none';
    $options['slide_counter'] = 'none';
    $options['show_image_titles'] = 'none';
    $options['items_per_page'] = 10;
    $options['effect'] = 'fade';
    $options['style'] = 'dark';
    $options['size'] = 600;
    return $options;
  }

  /**
   * Implementation of boxes_content::options_form().
   */
  public function options_form(&$form_state) {
    $private = os_get_bundles(array(0));
    if (isset($private['media_gallery'])) {
      return array(
        '#type' => 'markup',
        '#prefix' => '<div class="warning messages">',
        '#markup' => t('You must enable the !link to create a slideshow.', array('!link' => l('Media Gallery app', 'cp/build/features'))),
        '#suffix' => '</div>',
      );
    }
    
    $form = parent::options_form($form_state);

    //get the media gallery nodes.
    $galleries = array();
    ctools_include('node', 'os');
    $efq = new OSNodeFieldQuery();
    $efq->propertyCondition('type', 'media_gallery');
    $result = $efq->execute();

    if (isset($result['node']) && count($result['node'])) {
      foreach(node_load_multiple(array_keys($result['node'])) as $node) {
        $galleries[$node->nid] = $node->title;
      }
    }

    if (count($galleries)) {
      $form['gallery_nid'] = array(
        '#type' => 'select',
        '#title' => t('Gallery'),
        '#description' => t('Select a gallery to use as a slideshow.'),
        '#options' => array('all' => t('All')) + $galleries,
        '#default_value' => $this->options['gallery_nid'],
      );
    } else {
      $form['no_gallery_message'] = array(
        '#type' => 'markup',
        '#markup' => '<div>' . t('You must create a gallery to use as a slideshow.') . '</div>',
      );
    }

    $form['new_gallery'] = array(
      '#type' => 'markup',
      '#markup' => l(t('Create new gallery'), 'node/add/media-gallery'), //overlay?
    );

    $form['display'] = array(
      '#type' => 'fieldset',
      '#title' => t('Display Options'),
      '#collapsible' => TRUE,
    	'#collapsed' => TRUE,
    );

    $form['display']['style'] = array(
      '#title' => t('Player Styles'),
      '#type' => 'select',
      '#options' => array('dark' => t('Dark'), 'light' => t('Light')),
      '#default_value' => $this->options['style'],
      //'#pre_render' => array('os_slideshow_box_style_thumbs'),
      '#attributes' => array('class' => array('thumbnails')),
      '#description' => t('Choose between a black or light grey player style.'),
    );

    $form['display']['size'] = array(
      '#type' => 'textfield',
      '#title' => t('Size'),
      '#default_value' => $this->options['size'],
      '#field_suffix' => 'px',
      '#description' => t('Enter the width of the slideshow in pixels. It should be between 400 and 960 pixels.'),
      '#element_validate' => array('os_boxes_slideshow_accepted_range'),
      '#attached' => array(
        'js' => array(
          drupal_get_path('module', 'os_boxes').'/plugins/os_boxes_slideshow/os_slideshow_form.js'
        )
      ),
    );

    $display_options = array('pager' => t('Show Pager'), 'controls' => t('Show Controls'), 'slide_counter' => t('Slide Counter'));
    foreach ($display_options as $option => $title)
    $form['display'][$option] = array(
      '#type' => 'select',
      '#title' => $title,
      //omitting top/bottom options, but leaving this here so we can add it back in when we're ready for new features.
      //'#options' => array('none' => t('No Display'), 'top' => t('Top'), 'bottom' => t('Bottom')),
      '#options' => array('none' => t('No'), 'bottom' => t('Yes')),
      '#default_value' => $this->options[$option],
    );

    $form['display']['show_image_titles'] = array(
      '#type' => 'select',
      '#title' => t('Show image titles'),
      //omitting top/bottom options, but leaving this here so we can add it back in when we're ready for new features.
      //'#options' => array('none' => t('No Display'), 'top' => t('Top'), 'bottom' => t('Bottom')),
      '#options' => array('none' => t('No'), 'top' => t('Yes')),
      '#default_value' => $this->options['show_image_titles'],
      );

    $form['display']['items_per_page'] = array(
      '#type' => 'textfield',
      '#title' => t('Number of items'),
      '#description' => t('Limit to this many images per slideshow.  Use 0 for no limit.'),
      '#default_value' => (int) $this->options['items_per_page'],
    );

    //get jquery effects from view (include file hsould arleady be loaded)
    $slideshow_form_full = $state = array();
    if (module_exists('views_slideshow_cycle')) {
      if (!function_exists('views_slideshow_cycle_views_slideshow_slideshow_type_form')) {
        require_once drupal_get_path('module', 'views_slideshow_cycle') . '/views_slideshow_cycle.views_slideshow.inc';
      }
      @views_slideshow_cycle_views_slideshow_slideshow_type_form($slideshow_form_full, $state, $view);
      $form['display']['effect'] = $slideshow_form_full['views_slideshow_cycle']['effect'];
      $form['display']['effect']['#default_value'] = $this->options['effect'];
    }

    return $form;
  }

  /**
   * Implementation of boxes_content::render().
   */
  public function render() {
    $block = parent::render();
    
    //no slideshow when media gallery is disabled
    $private = os_get_bundles(array(0));
    if (isset($private['media_gallery'])) {
      return $block;
    }
    
    drupal_add_css(drupal_get_path('module', 'os_boxes') . '/plugins/os_boxes_slideshow/theme/os-slideshow-' . $this->options['style'] . '.css');
    drupal_add_css(drupal_get_path('module', 'os_boxes').'/plugins/os_boxes_slideshow/os_boxes_slideshow.css');

    if ($view = views_get_view('os_image_gallery_slideshow')) {

      $view->init_display();
      $handler = $view->display_handler;
      $style_options = $handler->get_option('style_options');

      //add slideshow widgets
      foreach(array('pager', 'controls', 'slide_counter') as $option) {
        foreach($style_options['widgets'] as $position => $info) {
          $style_options['widgets'][$position]['views_slideshow_' . $option]['enable'] = ($position == $this->options[$option]);
        }
      }

      //limit to one gallery
      if (((int)$this->options['gallery_nid']) > 0) {
        $filters = $handler->get_option('filters');
        $filters['nid'] = array(
          'id' => 'nid',
          'table' => 'node',
          'field' => 'nid',
          'relationship' => 'file_to_node',
          'group_type' => 'group',
          'ui_name' => '',
          'operator' => '=',
          'group' => '1',
          'exposed' => FALSE,
          'expose' => array(),
          'value' => array('min'=>'', 'max'=>'', 'value'=>$this->options['gallery_nid']),
        );
        $handler->set_option('filters', $filters);
      }

      //effects
      $style_options['views_slideshow_cycle']['effect'] = $this->options['effect'];
      $handler->set_option('style_options', $style_options);

      //title
      if ($this->options['show_image_titles'] == 'none') {
        $fields = $handler->get_option('fields');
        $fields['media_title']['exclude'] = TRUE;
        $handler->set_option('fields', $fields);
      }

      //number of items
      if ((int)$this->options['items_per_page'] > 0) {
        $view->set_items_per_page((int) $this->options['items_per_page']);
      } else {
        $handler->set_option('use_pager', FALSE);
      }

      $output = $view->preview();

      // dynamic size adjustments
      $styles = image_styles();
      $prefix = 'slideshow_';
      $data = array();
      foreach ($styles as $name => $presets) {
        if (strpos($name, $prefix) === 0) {
          $dim = current($presets['effects']);
          $dim = $dim['data'];
          $data[$dim['width']] = $dim;
        }
      }
      ksort($data);

      foreach ($data as $width => $dim) {
        if ($this->options['size'] <= $width) {
          $ratio = $dim['height']/$dim['width'];
          $preset = $prefix.$width;
          break;
        }
      }

      $width = $this->options['size'];
      $height = $width * $ratio;

      // swap out the dimenions of the preset with the dimenions the user wants
      $patt_w = '/(<img [^>]* width=")([^"]*)("[^>]*>)/';
      $patt_h = '/(<img [^>]* height=")([^"]*)("[^>]*>)/';
      $patt_url = '/<img [^>]*src="([^"]*)"[^>]*>/';

      $output = preg_replace($patt_w, '${1}'.($width).'px'.'${3}', $output);
      $output = preg_replace($patt_h, '${1}'.($height).'px'.'${3}', $output);
      $matches = array();
      preg_match_all($patt_url, $output, $matches, PREG_SET_ORDER);
      foreach ($matches as $m) {
        $output = str_replace($m[1], image_style_url($preset, $m[1]), $output);
      }

      $block['content'] = $output;
    }

    return $block;
  }

  /**
   * Return the cache parameters for this box
   */
  public function cache_info() {

    return array();

  //  	//Clear any time the site is saved
  //    $cache_key = array(
  //      'os_og' => array('current' => 'current'),
  //    );
  //
  //    //Only cache twitter feeds for 15 minutes (max_lifespan)
  //    return array('type' => BLOCK_CACHE_CONTENT_CACHE, 'keys' => $cache_key, 'max_lifespan' => 900);
  }

}

/**
 * Ensures the value given is within acceptable ranges
 */
function os_boxes_slideshow_accepted_range($element, &$form_state) {
  if ($element['#value'] < 400 || $element['#value'] > 960) {
    form_error($element, t('Slideshow width out of bounds: Width should be between 400 and 960 pixels.'));
  }
  $element['#value'] = (int)$element['#value'];
}
