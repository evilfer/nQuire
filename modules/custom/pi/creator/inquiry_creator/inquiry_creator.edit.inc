<?php

function inquiry_creator_new() {
  include_once(drupal_get_path('module', 'node') . '/node.pages.inc');

  $output = '';
  $output .= node_add('inquiry_design');

  return $output;
}

function inquiry_creator_edit($nid) {
  include_once(drupal_get_path('module', 'node') . '/node.pages.inc');

  $node = node_load($nid);

  $output = '';
  $output .= drupal_get_form('inquiry_creator_metadata_form', $node);

  return $output;
}

function inquiry_creator_main($nid, $op = '', $item = '') {
  $design = node_load($nid);
  drupal_set_title($design->title);

  $output = theme('inquiry_creator_inquiry_main', $design, $op, $item);

  switch ($op) {
    case '':

      $output .= '<p>' . t('Click on the edit link beside each activity or phase to edit their details.') . '</p>'
              . theme('inquiry_structure_widget_view', $design, $item);
      break;
    case 'structure': /* activity structure is being edited */
      $output .= drupal_get_form('inquiry_creator_activities_form', $design);
      break;
    case 'stages':
      $output .= drupal_get_form('inquiry_creator_stages_form', $design);
      break;
    case 'groups':
      $output .= drupal_get_form('inquiry_creator_groups_form', $design);
      break;
    case 'roles':
      $output .= drupal_get_form('inquiry_creator_roles_form', $design);
      break;
    default: /* other things */
      break;
  }

  return $output;
}

function theme_inquiry_creator_inquiry_metadata($node, $editable) {
  $output = '<p>';
  $output .= t('Title:') . '&nbsp<b>' . $node->title . '</b>';
  $output .= '<br/>' . t('Description:') . '&nbsp<b>' . $node->body . '</b>';
  if ($editable) {
    $output .= '<br/><small>' . nquire_commons_form_submit_link(t('edit'), "creator/{$node->nid}/edit") . '</small>';
  }
  $output .= '<br/>';

  return $output;
}

function theme_inquiry_creator_inquiry_main($node, $current_op = '') {
  $output = '';

  $output .= theme('inquiry_creator_inquiry_metadata', $node, TRUE);
  $output .= theme('inquiry_creator_edit_menu', $node, $current_op);

  return $output;
}

function theme_inquiry_creator_edit_menu($node, $current_op) {
  drupal_add_css(drupal_get_path('module', 'inquiry_creator') . '/css/editMenu.css');
  nquire_commons_load_submit_form_links();

  $ops = array(
      '' => t('Edit activities'),
      'structure' => t('Edit structure'),
      'stages' => t('Edit temporal organization'),
      'groups' => t('Edit collaboration organization'),
      'roles' => t('Edit roles'),
  );

  $output = '<div>';

  $output .='<table class="edit-link-menu"><tr>';
  foreach ($ops as $key => $title) {
    $output .= '<td class="edit-link-container' . ($key === $current_op ? ' selected' : ' active') . '">';

    if ($key === $current_op) {
      $output .= "<b>$title</b>";
    } else {
      $output .= nquire_commons_form_submit_link($title, "creator/{$node->nid}/{$key}");
    }

    $output .= '</td>';
  }

  $output .= '</table>';
  $output .= '</div>';

  return $output;
}

function theme_inquiry_creator_side_form($design, $item, $type, $form) {
  nquire_commons_load_javascript_module_support();
  nquire_commons_load_ahah_fix();

  drupal_add_js(drupal_get_path('module', 'inquiry_creator') . '/js/editFormPosition.js');
  drupal_add_css(drupal_get_path('module', 'inquiry_creator') . '/css/sideForm.css');

  $output = '<table class="item-edit-table"><tr><td>'
          . theme('inquiry_structure_widget_view', $design, $item)
          . '</td><td><div class="item-edit-form-container item-edit-form-container-' . $type . '">'
          . $form
          . '</div></td></tr></table>';

  return $output;
}

/* forms */

function _inquiry_creator_create_inquiry_id_form_item($node, &$form) {
  $form['inquiry_design'] = array(
      '#type' => 'hidden',
      '#default_value' => $node->nid,
      '#required' => TRUE,
  );
}

function inquiry_creator_create_form_save_cancel_buttons(&$form, $save_callback, $cancel_path) {
  inquiry_creator_create_form_buttons($form, array(
      'submit' => array(
          'type' => 'button',
          'label' => t('Save'),
          'callback' => $save_callback,
      ),
      'cancel' => array(
          'type' => 'link',
          'label' => t('Cancel'),
          'callback' => $cancel_path,
      ),
  ));
  inquiry_creator_create_form_destination($form, $cancel_path);
}

function inquiry_creator_create_form_delete_cancel_buttons(&$form, $delete_callback, $cancel_path) {
  inquiry_creator_create_form_buttons($form, array(
      'delete' => array(
          'type' => 'button',
          'label' => t('Delete'),
          'callback' => $delete_callback,
      ),
      'cancel' => array(
          'type' => 'link',
          'label' => t('Cancel'),
          'callback' => $cancel_path,
      ),
  ));
  inquiry_creator_create_form_destination($form, $cancel_path);
}

function inquiry_creator_create_form_save_delete_cancel_buttons(&$form, $save_callback, $delete_callback, $cancel_path) {
  inquiry_creator_create_form_buttons($form, array(
      'submit' => array(
          'type' => 'button',
          'label' => t('Save'),
          'callback' => $save_callback,
      ),
      'delete' => array(
          'type' => 'button',
          'label' => t('Delete'),
          'callback' => $delete_callback,
      ),
      'cancel' => array(
          'type' => 'link',
          'label' => t('Cancel'),
          'callback' => $cancel_path,
      ),
  ));
  inquiry_creator_create_form_destination($form, $cancel_path);
}

function inquiry_creator_create_form_destination(&$form, $destination) {
  $form['form_destination'] = array(
      '#type' => 'hidden',
      '#default_value' => $destination,
      '#required' => FALSE,
  );
}

function inquiry_creator_create_form_buttons(&$form, $buttons) {
  foreach ($buttons as $button => $data) {
    switch ($data['type']) {
      case 'button':
        $form[$button] = array(
            '#type' => 'submit',
            '#value' => $data['label'],
            '#submit' => array($data['callback']),
        );
        break;
      case 'link':
        $form[$button] = array(
            '#type' => 'markup',
            '#value' => l($data['label'], $data['callback']),
        );
        break;
    }
  }
}

function inquiry_creator_form_close($form, $form_state) {
  drupal_goto($form_state['values']['form_destination']);
}

function inquiry_creator_activities_form(&$form_state, $node) {
  $form = array();

  nquire_commons_form_set_default_link_action($form, 'submit');

  _inquiry_creator_create_inquiry_id_form_item($node, $form);
  inquiry_structure_widget_create_form_item($form, 'structure', $node->structure);
  inquiry_creator_create_form_save_cancel_buttons($form, 'inquiry_creator_activities_form_submit', "creator/{$node->nid}/");

  return $form;
}

function inquiry_creator_activities_form_submit($form, $form_state) {
  module_load_include('inc', 'inquiry_creator', 'inquiry_creator.phases');
  module_load_include('inc', 'inquiry_creator', 'inquiry_creator.activities');

  global $user;
  $node = node_load($form_state['values']['inquiry_design']);
  if ($node && $node->type === 'inquiry_design' && $node->uid === $user->uid) {
    $phases = json_decode($form_state['values']['structure']);

    foreach ($phases as $phase) {
      if (!is_numeric($phase->id)) {
        $phase_node = inquiry_creator_create_phase_stub($phase->title);
        $phase->id = $phase_node->nid;
      }

      foreach ($phase->activities as $activity) {
        if (!is_numeric($activity->id)) {
          $activity_node = inquiry_creator_create_activity_stub($activity->type, $activity->title);
          $activity->id = $activity_node->nid;
        }
      }
    }

    $new_structure = json_encode($phases);
    if ($node->structure !== $new_structure) {
      $node->structure = $new_structure;
      $node->revision = TRUE;
      node_save($node);
    }
  }
  inquiry_creator_form_close($form, $form_state);
}

function inquiry_creator_groups_form(&$form_state, $node) {
  $form = array();
  nquire_commons_form_set_default_link_action($form, 'submit');

  _inquiry_creator_create_inquiry_id_form_item($node, $form);
  inquiry_structure_groups_table_create_form_item($form, 'groups', $node->structure, $node->groups);
  inquiry_creator_create_form_save_cancel_buttons($form, 'inquiry_creator_groups_form_submit', "creator/{$node->nid}/");

  return $form;
}

function inquiry_creator_groups_form_submit($form, $form_state) {
  global $user;
  $node = node_load($form_state['values']['inquiry_design']);
  if ($node && $node->type === 'inquiry_design' && $node->uid === $user->uid) {

    if ($node->groups !== $form_state['values']['groups']) {
      $node->groups = $form_state['values']['groups'];
      $node->revision = TRUE;
      node_save($node);
    }
  }
  inquiry_creator_form_close($form, $form_state);
}

function inquiry_creator_roles_form(&$form_state, $node) {
  $form = array();

  nquire_commons_form_set_default_link_action($form, 'submit');

  _inquiry_creator_create_inquiry_id_form_item($node, $form);
  inquiry_structure_roles_table_create_form_item($form, 'roles', $node->structure, $node->roles);
  inquiry_creator_create_form_save_cancel_buttons($form, 'inquiry_creator_roles_form_submit', "creator/{$node->nid}/");

  return $form;
}

function inquiry_creator_roles_form_submit($form, $form_state) {
  global $user;
  $node = node_load($form_state['values']['inquiry_design']);
  if ($node && $node->type === 'inquiry_design' && $node->uid === $user->uid) {
    $roles = json_decode($form_state['values']['roles']);
    $id = 0;
    foreach ($roles as $role) {
      if (is_numeric($role->id)) {
        $id = max($id, $role->id);
      }
    }

    foreach ($roles as $role) {
      if (!is_numeric($role->id)) {
        $role->id = ++$id;
      }
    }

    $new_roles = json_encode($roles);
    if ($node->roles !== $new_roles) {
      $node->roles = $new_roles;
      $node->revision = TRUE;
      node_save($node);
    }
  }
  inquiry_creator_form_close($form, $form_state);
}

function inquiry_creator_stages_form(&$form_state, $node) {
  $form = array();

  nquire_commons_form_set_default_link_action($form, 'submit');

  _inquiry_creator_create_inquiry_id_form_item($node, $form);
  inquiry_structure_stages_table_create_form_item($form, 'stages', $node->structure, $node->stages);
  inquiry_creator_create_form_save_cancel_buttons($form, 'inquiry_creator_stages_form_submit', "creator/{$node->nid}/");

  return $form;
}

function inquiry_creator_stages_form_submit($form, $form_state) {
  global $user;
  $node = node_load($form_state['values']['inquiry_design']);
  if ($node && $node->type === 'inquiry_design' && $node->uid === $user->uid) {
    $stages = json_decode($form_state['values']['stages']);
    $id = 0;
    foreach ($stages as $stage) {
      if (is_numeric($stage->id)) {
        $id = max($id, $stage->id);
      }
    }

    foreach ($stages as $stage) {
      if (!is_numeric($stage->id)) {
        $stage->id = ++$id;
      }
    }

    $new_stages = json_encode($stages);
    if ($node->stages !== $new_stages) {
      $node->stages = $new_stages;
      $node->revision = TRUE;
      node_save($node);
    }
  }
  inquiry_creator_form_close($form, $form_state);
}

function inquiry_creator_metadata_form(&$form_state, $node) {
  $form = array();

  nquire_commons_form_set_default_link_action($form, 'submit');

  _inquiry_creator_create_inquiry_id_form_item($node, $form);

  $type = node_get_types('type', $node);

  $form['title'] = array(
      '#type' => 'textfield',
      '#title' => check_plain($type->title_label),
      '#description' => t('Write here the title for the inquiry.'),
      '#required' => TRUE,
      '#default_value' => $node->title,
  );

  //Show a simple textarea, no input selector required....
  $form['body_filter']['body'] = array(
      '#type' => 'textarea',
      '#title' => check_plain($type->body_label),
      '#description' => t('An introduction to the inquiry, e.g. about the goals of the inquiry, the scientific field etc.'),
      '#default_value' => $node->body,
      '#required' => TRUE
  );
  
  inquiry_creator_create_form_save_cancel_buttons($form, 'inquiry_creator_metadata_form_submit', "creator/{$node->nid}/");

  return $form;
}

function inquiry_creator_metadata_form_submit($form, $form_state) {
  global $user;
  $node = node_load($form_state['values']['inquiry_design']);
  if ($node && $node->type === 'inquiry_design' && $node->uid === $user->uid) {

    if ($node->title !== $form_state['values']['title'] || $node->body !== $form_state['values']['body']) {
      $node->title = $form_state['values']['title'];
      $node->body = $form_state['values']['body'];
      $node->revision = TRUE;
      node_save($node);
    }
  }
  inquiry_creator_form_close($form, $form_state);
}
