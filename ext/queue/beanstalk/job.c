
/*
  +------------------------------------------------------------------------+
  | Phalcon Framework                                                      |
  +------------------------------------------------------------------------+
  | Copyright (c) 2011-2013 Phalcon Team (http://www.phalconphp.com)       |
  +------------------------------------------------------------------------+
  | This source file is subject to the New BSD License that is bundled     |
  | with this package in the file docs/LICENSE.txt.                        |
  |                                                                        |
  | If you did not receive a copy of the license and are unable to         |
  | obtain it through the world-wide-web, please send an email             |
  | to license@phalconphp.com so we can send you a copy immediately.       |
  +------------------------------------------------------------------------+
  | Authors: Andres Gutierrez <andres@phalconphp.com>                      |
  |          Eduar Carvajal <eduar@phalconphp.com>                         |
  +------------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_phalcon.h"
#include "phalcon.h"

#include "Zend/zend_operators.h"
#include "Zend/zend_exceptions.h"
#include "Zend/zend_interfaces.h"

#include "kernel/main.h"
#include "kernel/memory.h"

#include "kernel/object.h"
#include "kernel/concat.h"
#include "kernel/fcall.h"
#include "kernel/array.h"
#include "kernel/operators.h"

/**
 * Phalcon\Queue\Beanstalk\Job
 *
 * Represents a job in a beanstalk queue
 */


/**
 * Phalcon\Queue\Beanstalk\Job initializer
 */
PHALCON_INIT_CLASS(Phalcon_Queue_Beanstalk_Job){

	PHALCON_REGISTER_CLASS(Phalcon\\Queue\\Beanstalk, Job, queue_beanstalk_job, phalcon_queue_beanstalk_job_method_entry, 0);

	zend_declare_property_null(phalcon_queue_beanstalk_job_ce, SL("_queue"), ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(phalcon_queue_beanstalk_job_ce, SL("_id"), ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(phalcon_queue_beanstalk_job_ce, SL("_body"), ZEND_ACC_PROTECTED TSRMLS_CC);

	return SUCCESS;
}

/**
 * Phalcon\Queue\Beanstalk\Job
 *
 * @param Phalcon\Queue\Beanstalk $queue
 * @param string $id
 * @param mixed $body
 */
PHP_METHOD(Phalcon_Queue_Beanstalk_Job, __construct){

	zval **queue, **id, **body;

	phalcon_fetch_params_ex(3, 0, &queue, &id, &body);
	PHALCON_VERIFY_CLASS_EX(*queue, phalcon_queue_beanstalk_ce, phalcon_exception_ce, 0);
	PHALCON_ENSURE_IS_STRING(id);

	phalcon_update_property_this(this_ptr, SL("_queue"), *queue TSRMLS_CC);
	phalcon_update_property_this(this_ptr, SL("_id"),    *id    TSRMLS_CC);
	phalcon_update_property_this(this_ptr, SL("_body"),  *body  TSRMLS_CC);
}

/**
 * Returns the job id
 *
 * @return string
 */
PHP_METHOD(Phalcon_Queue_Beanstalk_Job, getId){


	RETURN_MEMBER(this_ptr, "_id");
}

/**
 * Returns the job body
 *
 * @return mixed
 */
PHP_METHOD(Phalcon_Queue_Beanstalk_Job, getBody){


	RETURN_MEMBER(this_ptr, "_body");
}

/**
 * Removes a job from the server entirely
 *
 * @return boolean
 */
PHP_METHOD(Phalcon_Queue_Beanstalk_Job, delete){

	zval *id, *command, *queue, *response, *status;

	PHALCON_MM_GROW();

	id    = phalcon_fetch_nproperty_this(this_ptr, SL("_id"), PH_NOISY_CC);
	queue = phalcon_fetch_nproperty_this(this_ptr, SL("_queue"), PH_NOISY_CC);
	
	PHALCON_ALLOC_GHOST_ZVAL(command);
	PHALCON_CONCAT_SV(command, "delete ", id);
	phalcon_call_method_p1_noret(queue, "write", command);
	
	PHALCON_INIT_VAR(response);
	phalcon_call_method(response, queue, "readstatus");
	
	PHALCON_OBS_VAR(status);
	phalcon_array_fetch_long(&status, response, 0, PH_NOISY);
	if (PHALCON_IS_STRING(status, "DELETED")) {
		RETURN_MM_TRUE;
	}
	
	RETURN_MM_FALSE;
}

/**
 * The release command puts a reserved job back into the ready queue (and marks
 * its state as "ready") to be run by any client. It is normally used when the job
 * fails because of a transitory error.
 *
 * @return boolean
 */
PHP_METHOD(Phalcon_Queue_Beanstalk_Job, release){

	zval *priority = NULL, *delay = NULL;
	zval *id, *command, *queue, *response, *status;

	phalcon_fetch_params(0, 0, 2, &priority, &delay);

	PHALCON_MM_GROW();

	if (!priority) {
		PHALCON_INIT_VAR(priority);
		ZVAL_LONG(priority, 100);
	}

	if (!delay) {
		delay = PHALCON_GLOBAL(z_zero);
	}

	id    = phalcon_fetch_nproperty_this(this_ptr, SL("_id"), PH_NOISY_CC);
	queue = phalcon_fetch_nproperty_this(this_ptr, SL("_queue"), PH_NOISY_CC);

	PHALCON_ALLOC_GHOST_ZVAL(command);
	PHALCON_CONCAT_SVSVSV(command, "release ", id, " ", priority, " ", delay);
	phalcon_call_method_p1_noret(queue, "write", command);

	PHALCON_INIT_VAR(response);
	phalcon_call_method(response, queue, "readstatus");

	PHALCON_OBS_VAR(status);
	phalcon_array_fetch_long(&status, response, 0, PH_NOISY);
	if (PHALCON_IS_STRING(status, "RELEASED")) {
		RETURN_MM_TRUE;
	}

	RETURN_MM_FALSE;
}

/**
 * The bury command puts a job into the "buried" state. Buried jobs are put into
 * a FIFO linked list and will not be touched by the server again until a client
 * kicks them with the "kick" command.
 *
 * @return boolean
 */
PHP_METHOD(Phalcon_Queue_Beanstalk_Job, bury){

	zval *priority = NULL;
	zval *id, *command, *queue, *response, *status;

	phalcon_fetch_params(0, 0, 1, &priority);

	PHALCON_MM_GROW();

	if (!priority) {
		PHALCON_INIT_VAR(priority);
		ZVAL_LONG(priority, 100);
	}

	id    = phalcon_fetch_nproperty_this(this_ptr, SL("_id"), PH_NOISY_CC);
	queue = phalcon_fetch_nproperty_this(this_ptr, SL("_queue"), PH_NOISY_CC);

	PHALCON_ALLOC_GHOST_ZVAL(command);
	PHALCON_CONCAT_SVSV(command, "bury ", id, " ", priority);
	phalcon_call_method_p1_noret(queue, "write", command);

	PHALCON_INIT_VAR(response);
	phalcon_call_method(response, queue, "readstatus");

	PHALCON_OBS_VAR(status);
	phalcon_array_fetch_long(&status, response, 0, PH_NOISY);
	if (PHALCON_IS_STRING(status, "BURIED")) {
		RETURN_MM_TRUE;
	}

	RETURN_MM_FALSE;
}

/**
 * The bury command puts a job into the "buried" state. Buried jobs are put into
 * a FIFO linked list and will not be touched by the server again until a client
 * kicks them with the "kick" command.
 *
 * @return boolean
 */
PHP_METHOD(Phalcon_Queue_Beanstalk_Job, touch){

	zval *id, *command, *queue, *response, *status;

	PHALCON_MM_GROW();

	id    = phalcon_fetch_nproperty_this(this_ptr, SL("_id"), PH_NOISY_CC);
	queue = phalcon_fetch_nproperty_this(this_ptr, SL("_queue"), PH_NOISY_CC);

	PHALCON_ALLOC_GHOST_ZVAL(command);
	PHALCON_CONCAT_SV(command, "touch ", id);
	phalcon_call_method_p1_noret(queue, "write", command);

	PHALCON_INIT_VAR(response);
	phalcon_call_method(response, queue, "readstatus");

	PHALCON_OBS_VAR(status);
	phalcon_array_fetch_long(&status, response, 0, PH_NOISY);
	if (PHALCON_IS_STRING(status, "TOUCHED")) {
		RETURN_MM_TRUE;
	}

	RETURN_MM_FALSE;
}

/**
 * Move the job to the ready queue if it is delayed or buried.
 *
 * @return boolean
 */
PHP_METHOD(Phalcon_Queue_Beanstalk_Job, kick){

	zval *id, *command, *queue, *response, *status;

	PHALCON_MM_GROW();

	id    = phalcon_fetch_nproperty_this(this_ptr, SL("_id"), PH_NOISY_CC);
	queue = phalcon_fetch_nproperty_this(this_ptr, SL("_queue"), PH_NOISY_CC);

	PHALCON_ALLOC_GHOST_ZVAL(command);
	PHALCON_CONCAT_SV(command, "kick-job ", id);
	phalcon_call_method_p1_noret(queue, "write", command);

	PHALCON_INIT_VAR(response);
	phalcon_call_method(response, queue, "readstatus");

	PHALCON_OBS_VAR(status);
	phalcon_array_fetch_long(&status, response, 0, PH_NOISY);
	if (PHALCON_IS_STRING(status, "KICKED")) {
		RETURN_MM_TRUE;
	}

	RETURN_MM_FALSE;
}

PHP_METHOD(Phalcon_Queue_Beanstalk_Job, __wakeup) {

	zval *id    = phalcon_fetch_nproperty_this(this_ptr, SL("_id"), PH_NOISY_CC);
	zval *queue = phalcon_fetch_nproperty_this(this_ptr, SL("_queue"), PH_NOISY_CC);

	PHALCON_VERIFY_CLASS_EX(queue, phalcon_queue_beanstalk_ce, phalcon_exception_ce, 0);

	if (Z_TYPE_P(id) != IS_STRING) {
		zend_throw_exception_ex(phalcon_exception_ce, 0 TSRMLS_CC, "Unexpected inconsistency in %s - possible break-in attempt!", "Phalcon\\Queue\\Beanstalk\\Job::__wakeup()");
	}
}