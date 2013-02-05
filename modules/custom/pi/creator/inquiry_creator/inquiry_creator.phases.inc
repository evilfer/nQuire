<?php

function inquiry_creator_create_phase_stub($title) {
  global $user;

  $node = new stdClass();
  $node->type = 'inquiry_phase';
  $node->title = $title;
  $node->uid = $user->uid;

  node_save($node);
  return $node;
}

function inquiry_creator_phase_edit($design_nid, $phase_nid) {
  $design = node_load($design_nid);
  $node = node_load($phase_nid);

  $output = theme('inquiry_creator_inquiry_main', $design, 'activities');

  $form = drupal_get_form('inquiry_creator_phase_form', $design, $node);

  $output .= theme('inquiry_creator_side_form', $design, $phase_nid, 'phase', $form);
  return $output;
}

function _inquiry_creator_create_phase_id_form_item($phase, &$form) {
  $form['phase_id'] = array(
      '#type' => 'hidden',
      '#default_value' => $phase->nid,
      '#required' => TRUE,
  );
}

function inquiry_creator_phase_form(&$form_state, $design, $node) {
  $form = array();

  nquire_commons_form_set_default_link_action($form, 'submit');

  _inquiry_creator_create_inquiry_id_form_item($design, $form);
  _inquiry_creator_create_phase_id_form_item($node, $form);

  $type = node_get_types('type', $node);

  $form['title'] = array(
      '#type' => 'textfield',
      '#title' => check_plain($type->title_label),
      '#description' => t('Write here the name for the phase.'),
      '#required' => TRUE,
      '#default_value' => $node->title,
  );

  //Show a simple textarea, no input selector required....
  $form['body_filter']['body'] = array(
      '#type' => 'textarea',
      '#title' => check_plain($type->body_label),
      '#description' => t('A brief description of the phase objectives. This will help the inquiry participants understand their tasks.'),
      '#default_value' => $node->body,
      '#required' => TRUE
  );

  inquiry_creator_create_form_save_cancel_buttons($form, 'inquiry_creator_phase_form_submit', "creator/{$design->nid}/");

  return $form;
}

function inquiry_creator_phase_form_submit($form, $form_state) {
  $design = node_load($form_state['values']['inquiry_design']);
  $phase = node_load($form_state['values']['phase_id']);
  $title = $form_state['values']['title'];

  if ($design && $phase) {
    $changed = FALSE;

    if ($phase->title !== $form_state['values']['title']) {
      if (inquiry_design_set_item_title($design, $phase->nid, $title)) {
        $design->revision = TRUE;
        node_save($design);
      }
      $changed = TRUE;
    } else if ($phase->body !== $form_state['values']['body']) {
      $changed = TRUE;
    }

    if ($changed) {
      $phase->title = $form_state['values']['title'];
      $phase->body = $form_state['values']['body'];
      $phase->revision = TRUE;
      node_save($phase);
    }
  }

  inquiry_creator_form_close($form, $form_state);
}

function inquiry_creator_phase_delete($design_nid, $phase_nid) {
  $design = node_load($design_nid);
  $node = node_load($phase_nid);

  $output = theme('inquiry_creator_inquiry_main', $design, 'activities');

  $form = drupal_get_form('inquiry_creator_phase_delete_form', $design, $node);

  $output .= theme('inquiry_creator_side_form', $design, $phase_nid, 'phase', $form);
  return $output;
}

function inquiry_creator_phase_delete_form(&$form_state, $design, $node) {
  $form = array();

  _inquiry_creator_create_inquiry_id_form_item($design, $form);
  _inquiry_creator_create_phase_id_form_item($node, $form);

  $info = t('<h1 class="title">Are you sure to delete phase <i>@title</i>?</h1>', array('@title' => $node->title));
  $activities = inquiry_design_get_activities($design, $node->nid);
  if ($activities && count($activities > 0)) {
    $info .= '<p>' . t('The following activities will also be deleted:') . '<ul>';
    foreach ($activities as $activity) {
      $info .= '<li><b>' . check_plain($activity['title']) . '</b></li>';
    }
    $info .= '</ul></p>';
  }

  $info .= '<div>' . t('This action cannot be undone.') . '</div>';


  $form['info'] = array(
      '#value' => $info,
  );

  inquiry_creator_create_form_delete_cancel_buttons($form, 'inquiry_creator_phase_delete_form_submit', "creator/{$design->nid}");

  return $form;
}

function inquiry_creator_phase_delete_form_submit($form, $form_state) {
  global $user;

  $design = node_load($form_state['values']['inquiry_design']);
  $phase = node_load($form_state['values']['phase_id']);

  if ($phase && $phase->type === 'inquiry_phase' && $phase->uid == $user->uid &&
          $design && $design->type === 'inquiry_design' && $design->uid == $user->uid) {

    $phase_nid = $phase->nid;

    $activities = inquiry_design_get_activities($design, $phase_nid);
    foreach ($activities as $activity_def) {
      module_load_include('inc', 'inquiry_creator', 'inquiry_creator.activities');
      _inquiry_creator_activity_delete_node($activity_def['id']);
    }

    node_delete($phase_nid);

    if (inquiry_design_remove_structure_item($design, $phase_nid)) {
      node_save($design);
    }
  }

  inquiry_creator_form_close($form, $form_state);
}