<?php
/*
** Zabbix
** Copyright (C) 2001-2022 Zabbix SIA
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**/


/**
 * Class containing methods for operations with widget iterators.
 */
abstract class CControllerWidgetIterator extends CControllerDashboardWidgetView {

	protected function init(): void {
		parent::init();

		$this->addValidationRules([
			'page' => 'required|ge 1'
		]);
	}

	/**
	 * Get realistic page number for given number of child widgets.
	 */
	protected function getIteratorPage(int $num_widgets): int {
		return max(1, min((int) $this->getInput('page'), $this->getIteratorPageCount($num_widgets)));
	}

	/**
	 * Get number of child widgets to show on a single page.
	 */
	protected function getIteratorPageSize(): int {
		$fields_data = $this->getForm()->getFieldsValues();

		return min($fields_data['rows'] * $fields_data['columns'],
			floor(DASHBOARD_MAX_COLUMNS * DASHBOARD_WIDGET_MAX_ROWS / DASHBOARD_WIDGET_MIN_ROWS)
		);
	}

	/**
	 * Get number of pages for given number of child widgets.
	 */
	protected function getIteratorPageCount(int $num_widgets): int {
		return (floor(max(0, $num_widgets - 1) / $this->getIteratorPageSize()) + 1);
	}
}
