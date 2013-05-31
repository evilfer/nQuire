<?php

function pi_activities_page($node) {
	global $user;

	$inquiry_info = pi_info()->getCurrentInquiryInfo();
	$inquiry_access = pi_info()->getAccessManager($inquiry_info->getInquiryNid(), $user->uid);

	if ($node->type === 'pi_activity') {
		$phase_nid = $node->phase_nid;
		$activity_edit = $node->nid;

		if ($node->parent_activity) {
			$separate_f = "{$node->activity_type}_edit_in_separate_page";
			if (function_exists($separate_f) && $separate_f($node)) {
				$mode = 'activity_page';
			} else {
				$mode = 'child_activity_edit';
				$parent_activity = $node->parent_activity;
			}
		} else {
			$mode = 'parent_activity_edit';
			$parent_activity = FALSE;
		}
	} else {
		$phase_nid = $node->nid;
		$mode = 'phase_view';
		$activity_edit = FALSE;
	}

	$phase_count = count($inquiry_info->getPhases());
	$phase_index = $inquiry_info->getPhaseIndex($phase_nid);
	$keys = _ag_phase_keys($phase_count);
	$phase_key = $keys[$phase_index % strlen($keys)];
	$phase_node = $mode === 'phase_view' ? $node : $inquiry_info->getPhase($phase_nid);

	$data = array(
			'in_phase' => $mode !== 'activity_page',
			'phase' => array(
					'node' => $phase_node,
			),
			'activities' => array(),
	);


	if ($data['in_phase']) {
		$data['phase']['phase_key'] = $phase_key;
		$data['phase']['title'] = check_plain($phase_node->title);
		$data['phase']['description'] = nl2br(check_plain($phase_node->body));

		$link = l(t('here'), 'phase/' . $phase_node->nid . '/shared');

		switch ($phase_node->collaboration) {
			case 'individual':
				$data['phase']['sharing'] .= t('Activities in this phase are carried out individually. ')
								. t('Click !link to see what other participants are doing.', array('!link' => $link));
				break;
			case 'group':
				$data['phase']['sharing'] .= t('Activities in this phase are carried out in groups.')
								. t('Click !link to see what other groups are doing.', array('!link' => $link));
				break;
			default:
				$data['phase']['sharing'] .= t('Activities in this phase are done by all inquiry participants working together.');
				break;
		}
	}


	$activities = $mode === 'activity_page' ? array($node->nid => $node) : $inquiry_info->getActivitiesForPhase($phase_nid);

	foreach ($activities as $activity_nid => $activity_node) {
		$edit = $mode == 'parent_activity_edit' && $activity_edit === $activity_nid;
		$child_edit = $mode = 'child_activity_edit' && $parent_activity === $activity_nid;

		$data['activities'][$activity_nid] = _pi_activities_get_activity_data($inquiry_info, $inquiry_access, $user, $phase_key, $activity_node, $edit, $child_edit);
	}

	$output = theme('pi_activities_view_phase', $data);

	return $output;
}

function _pi_activities_get_activity_data($inquiry_info, $inquiry_access, $user, $phase_key, $activity_node, $edit, $edit_child) {
	$access = $inquiry_access->getAccessToItem($activity_node);
	$reason = $inquiry_access->getAccessReason($activity_node);
	$inquiry_nid = $inquiry_info->getInquiryNid();
	$exaplanation = pi_activity_access_get_explanation_text($access, $reason, $inquiry_nid);

	$data = array(
			'node' => $activity_node,
			'title' => check_plain($activity_node->title),
			'description' => nl2br(check_plain($activity_node->body)),
			'links' => $access === 'edit' ? pi_activity_create_edit_links($activity_node, $edit) : NULL,
			'access' => $access,
			'reason' => $reason,
			'access_explanation' => $exaplanation,
			'can_view' => $edit ? pi_activity_tab_edit_access_callback($activity_node) : pi_activity_tab_view_access_callback($activity_node),
			'edit_child' => $edit_child,
			'edit' => $edit,
			'phase_key' => $phase_key,
	);

	if ($data['can_view']) {
		$content_manager = pi_info()->getContentManager();
		$content_nid = $content_manager->getContent($activity_node->nid, $user->uid, TRUE);
		$content_node = node_load($content_nid);

		if ($edit) {
			if (!$content_node) {
				$content_node = new stdClass;
				$content_node->uid = $user->uid;
				$content_node->type = $activity_node->activity_type;
			}
			$data['content'] = array(
					'type' => 'single_form',
					'form' => node_page_edit($content_node),
			);
		} else {
			$view_f = "{$activity_node->activity_type}_display_content_in_activity";
			$data['content'] = $view_f($content_node, $edit_child);
		}
	}

	return $data;
}


function theme_pi_activities_view_phase($data) {
	$output = '';
	return $output;
}

function theme_pi_activities_view_activity($activity_data) {
	$output = '';
	return $output;
}