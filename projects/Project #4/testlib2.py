# -*- coding: UTF-8 -*-

import os
import re
import time
import shutil
import subprocess


# Name of file with settings
settings_filename = 'Settings.ini'

# Paths to components
path_to_solution = '---- CUT! ----'
path_to_project = path_to_solution + '---- CUT! ----'
path_to_settings = path_to_project + settings_filename
path_to_builded = path_to_project + '---- CUT! ----'
path_to_program = path_to_builded + '---- CUT! ----'
path_to_result_dir = u'\\\\?\\{}\\ResultDir\\'.format(path_to_solution)

# Files to be saved into the directory for results after program executed
files_for_save = list([

# ---- CUT! ----

])

# Files to be deleted after program executed
files_for_delete = list([

# ---- CUT! ----

])


def sign(a):
    return (a >= 0) - (a < 0)


class Log:

    def __init__(self, log_filename='log.txt', mode='a'):
        self.log_filename = log_filename
        self.log_filename_mode = mode
        self.error = False

        try:
            self.log_file = open(self.log_filename, self.log_filename_mode)
        except IOError:
            self.log_file = ''
            self.error = True

    def __del__(self):
        self.close()

    def q(self, message):
        message_p = "{}: {}".format(
            time.strftime('%Y%m%d_%H%M%S', time.localtime()),
            message
        )
        message = message_p + '\r\n'

        print message_p

        if not self.error:
            self.log_file.write(message)
            self.log_file.flush()

    def close(self):
        if not self.error:
            self.error = True
            self.log_file.write('\r\n\r\n\r\n')
            self.log_file.close()

    def sep(self):
        self.q('-----------------------')


class Parameter:
    __parameter = None
    __begin = None
    __end = None
    __n = None
    __step = None
    __value = None
    __i = None

    def __init__(self, param, begin, end, n=None, step=None):
        if type(begin) is not int and type(begin) is not float:
            raise TypeError('Parameter(): type of begin is int or float')
        if type(end) is not int and type(end) is not float:
            raise TypeError('Parameter(): type of end is int or float')

        if n is not None:
            if type(n) is not int or n < 1:
                raise TypeError('Parameter(): n, only positive integers are accepted')
        self.__n = n

        if step is not None:
            if type(step) is not int and type(step) is not float or step == 0:
                raise TypeError('Parameter(): step, only int and float which is not zero accepted')
            if begin < end and step < 0:
                raise ValueError('Parameter(): step < 0 when begin < end, check values')
            if begin > end and step > 0:
                raise ValueError('Parameter(): step > 0 when begin > end, check values')
            self.__n = int((end - begin) / step) + 1
        self.__step = step

        if self.__step is None and self.__n is None:
            raise ValueError('Parameter(): step and n both are None')

        if type(param) is not str:
            raise TypeError('Parameter(): param, only str accepted')

        self.__parameter = param
        self.__begin = begin
        self.__end = end

        if self.__step is None:
            self.__step = self.__end - self.__begin
            self.__step /= (self.__n - 1) if self.__n > 1 else 1

        self.reset()

    def __iter__(self):
        return self

    def __str__(self):
        return "Parameter(param='{}', begin={}, end={}, n={}, step={})".format(
            self.__parameter, self.__begin, self.__end, self.__n, self.__step
        )

    def __next__(self):
        return self.next()

    def next(self):
        sg = sign(self.__step)
        if self.__i < self.__n:
            if sg*(self.__value + self.__step) > sg*self.__end:
                current = self.__end
            else:
                current = self.__value

            self.__value += self.__step
            self.__i += 1
            return current
        else:
            self.reset()
            raise StopIteration()

    def value(self):
        return self.__value

    def values(self):
        res = []
        i = 0
        v = self.__begin
        sg = sign(self.__step)

        while True:
            if i < self.__n:
                if sg*(v + self.__step) > sg*self.__end:
                    current = self.__end
                else:
                    current = v

                v += self.__step
                i += 1
                res.append(current)
            else:
                break
        return res

    def name(self):
        return self.__parameter

    def reset(self):
        self.__value = self.__begin
        self.__i = 0

    def begin(self):
        return self.__begin

    def end(self):
        return self.__end

    def step(self):
        return self.__step


def print_m(m):
    print m


def set_parameter_in_file(filename, parameter, value):
    settings_file = open(filename)
    settings = settings_file.readlines()
    settings_file.close()

    for line in settings:
        settings[settings.index(line)] = re.sub(
            r'^('+parameter+r'\s*=\s*)([\w\+\-\.]*)(\s*;?.*)?$',
            r'\g<1>'+str(value)+r'\g<3>',
            line
        )

    settings_file = open(filename, 'w')
    settings_file.writelines(settings)
    return settings


def set_parameter(parameter, value):
    return set_parameter_in_file(path_to_settings, parameter, value)


def parameter_test(parameter, directory=None, log=print_m):
    start_time = time.time()

    timestamp = time.strftime('%Y%m%d%H%M%S', time.localtime())
    if directory is None:
        directory = path_to_result_dir + '{}_parameter_test_{}'.format(timestamp, parameter.name())

    log('Parameter test is started.')
    try:
        os.makedirs(directory)
    except OSError as e:
        log('[ERROR] Cannot create directory for results ({}) - {}.'.format(
            repr(directory), os.strerror(e.errno)
        ))
        return 1
    except Exception as e:
        log('[ERROR] Cannot create directory for results ({}) - UnErr ({}).'.format(
            repr(directory), e.message
        ))
        return 1
    log('Directory for results is created: {}.'.format(directory))
    directory += '\\'

    # main cycle for experiment
    i = 1
    for value in parameter.values():
        log('Iteration {}: {}={}.'.format(i, parameter.name(), value))

        # setting the value of parameter given as arguments in file with settings
        try:
            set_parameter(parameter.name(), value)
        except OSError as e:
            log('[ERROR] Cannot set parameter is settings file ({}) - {}.'.format(
                path_to_settings, os.strerror(e.errno)
            ))
            return 1
        except Exception as e:
            log('[ERROR] Cannot set parameter is settings file ({}) - UnErr ({}).'.format(
                path_to_settings, e.message
            ))
            return 1

        # executing of program
        log('Execution of program.')
        prev_dir = os.getcwd()
        os.chdir(path_to_builded)
        try:
            process = subprocess.Popen(path_to_program)
            process.communicate()
        except Exception as e:
            log('[ERROR] Cannot execute the program ({}) - UnErr ({}).'.format(
                path_to_program, e.message
            ))
            return 1

        os.chdir(prev_dir)
        log('Execution finished.')

        # copying file with settings into directory for results
        settings_source = path_to_settings
        settings_destination = directory + "{}_{}".format(i, settings_filename)

        try:
            shutil.copy(settings_source, settings_destination)
        except IOError as e:
            log('[ERROR] Cannot copy file with settings ({}) into directory for results ({}) - {}'.format(
                settings_source, settings_destination, os.strerror(e.errno)
            ))
        except Exception as e:
            log('[ERROR] Cannot copy file with settings ({}) into directory for results ({}) - UnErr ({})'.format(
                settings_source, settings_destination, e.message
            ))

        log('Copy of settings file is saved in directory for results ({}).'.format(settings_destination))

        # moving files with results of experiment from files_for_save list into directory for results
        for filename in files_for_save:
            source = path_to_builded + filename
            destination = directory + "{}_{}".format(i, filename)

            try:
                shutil.move(source, destination)
            except IOError as e:
                log('Cannot copy results of experiment ({}) into directory for results ({}) - {}.'.format(
                    source, destination, os.strerror(e.errno)
                ))
            except Exception as e:
                log('Cannot copy results of experiment ({}) into directory for results ({}) - UnErr {}.'.format(
                    source, destination, e.message
                ))

        # removing files with results of experiment from files_for_delete list
        for filename in files_for_delete:
            source = path_to_builded + filename
            try:
                os.remove(source)
            except IOError as e:
                log('Cannot remove results of experiment ({}) - {}.'.format(
                    source, os.strerror(e.errno)
                ))
            except Exception as e:
                log('Cannot remove results of experiment ({}) - UnErr {}.'.format(
                    source, e.message
                ))

        # increasing iteration counter
        i += 1

    finish_time = time.time()
    log('Parameter test finished. Seconds elapsed: {}.'.format(finish_time - start_time))
    return 0


def n_parameters_test(*args, **kwargs):
    start_time = time.time()
    errors = 0

    if 'log' in kwargs:
        log = kwargs['log']
    else:
        log = print_m

    if 'res_dir' in kwargs:
        res_dir = kwargs['res_dir']
    else:
        res_dir = None

    parameters = []

    i = 1
    for arg in args:
        if isinstance(arg, Parameter):
            parameters.append(arg)
            log('param_{} = {}.'.format(i, arg))
        else:
            errors += 1
            log('[ERROR] n_parameters_test() given non-Parameter argument no_{}!'.format(i))
        i += 1

    if len(parameters) == 0:
        log('Have no parameters to test. Exit.')
    else:
        log_name = 'n{}_{}_{}-{}.txt'.format(
            len(parameters), parameters[0].name(), parameters[0].begin(), parameters[0].end()
        )
        log_path = path_to_solution + log_name

        timestamp = time.strftime('%Y%m%d%H%M%S', time.localtime())
        if res_dir is None:
            dir_name = path_to_result_dir + '{}_n_parameters_test_{}'.format(
                timestamp, len(parameters)
            )
        else:
            dir_name = res_dir

        log_p = Log(log_path)
        log('Starting n_parameters_test.')
        log('Work dir: {}.'.format(dir_name))

        if len(parameters) == 1:
            work_dir = dir_name
            log('Work dir for subtest: {}.'.format(work_dir))
            log('Starting parameter test...')
            res = parameter_test(parameters[0], directory=work_dir, log=log_p.q)
            if res == 0:
                log('parameter_test finished successfully.')
            else:
                errors += res
                log('[ERROR] parameter_test finished with errors. See {} for more information'.format(log_name))
        else:
            i = 1
            for value in parameters[0].values():
                log('Iteration {}: {}={}.'.format(str(i)*len(parameters), parameters[0].name(), value))
                log_p.q('Iteration {}: {}={}.'.format(str(i)*len(parameters), parameters[0].name(), value))
                set_parameter_in_file(path_to_settings, parameters[0].name(), value)
                work_dir = dir_name + '\\{}={}'.format(
                    parameters[0].name(), value
                )
                log('Running n_parameters_test...')
                res = n_parameters_test(res_dir=work_dir, log=log_p.q, *args[1:])
                if res == 0:
                    log('n_parameters_test finished successfully.')
                else:
                    errors += res
                    log('n_parameters_test finished with errors. See {} for more information'.format(log_name))
                i += 1
                log_p.sep()
        log_p.close()

        if errors != 0:
            log('Test finished with errors. See {} for more information.'.format(log_name))
        else:
            log('Test finished successfully.')
        log('Copying log file into work dir...')

        log_source = log_path
        log_destination = "{}\\{}".format(dir_name, log_name)

        try:
            shutil.move(log_source, log_destination)
        except IOError as e:
            log('[ERROR] Cannot copy log of experiment ({}) into directory for results ({}) - {}.'.format(
                log_source, log_destination, os.strerror(e.errno)
            ))
            errors += 1
        except Exception as e:
            log('[ERROR] Cannot copy log of experiment ({}) into directory for results ({}) - UnErr {}.'.format(
                log_source, log_destination, e.message
            ))
            errors += 1
        log('Done.')
    log('{} errors. Exit.'.format(errors))

    finish_time = time.time()
    log('n_parameters_test finished. Seconds elapsed: {}.'.format(finish_time - start_time))

    return errors