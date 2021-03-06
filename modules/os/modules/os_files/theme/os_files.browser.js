/**
 * 
 */

(function ($, undefined) {
  Drupal.behaviors.osFilesMediaBrowser = {
    attach: function (ctx, s) {
      var $items = $('.media-item:not(.os-files-processed)', ctx),
          forms = {};
      
      $items.each(function () {
        var base = this.id,
            fid = this.getAttribute('data-fid'),
            settings = {
              url: s.basePath+('pathPrefix' in s?s.pathPrefix:'')+'file/'+fid+'/edit/nojs',
              event: 'click',
              wrapper: 'file-edit-section',
              method: 'html',
              progress: {type: 'throbber'}
            },
            ajax = new Drupal.ajax(base, this, settings);
       $(this).addClass('os-files-processed');
      });
      
      Drupal.ajax.prototype.beforeSubmit = function (values, element, options) {
        if (this.wrapper == '#undefined') {
          var form = $(this.selector).parents('form');
          if (form.length) {
            this.wrapper = '#'+form[0].id;
          }
        }
      };
      
      $('#media-browser-tabset').tabs('option', 'select', function (event, ui) {
        if (ui.tab.hash) {
          window.location.hash = ui.tab.hash;
        }
      });
      
      $('.ctools-auto-submit-full-form').submit(function (e) {e.preventDefault();});
      
    }
  };
})(jQuery, undefined);