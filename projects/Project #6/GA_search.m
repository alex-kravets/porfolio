function [resulting_individuals, evolution_history] = ga_search(varargin)
    warning off; close all; clc;
    
	if nargin >= 1
		received_parameters = varargin{1};
		
		parameters_struct	= received_parameters.parameters_struct;
		parameters_to_find	= received_parameters.parameters_to_find;
		
		population_size		= received_parameters.population_size;
		max_iterations		= received_parameters.max_iterations;
		max_combinations	= received_parameters.max_combinations;
		lucky_amount		= received_parameters.lucky_amount;
		cataclysm_power		= received_parameters.cataclysm_power;
		max_cataslysms		= received_parameters.max_cataslysms;
		realtime_count		= received_parameters.realtime_count;
		max_individuals		= received_parameters.max_individuals;
		
		max_tau				= received_parameters.max_tau;
		min_tau				= received_parameters.min_tau;
		
		% --- ѕараметры финального отображени€ графиков ---
		show_selected_individuals = false;
		show_final_profiles = false;
		show_evolution_history = false;
	else
		% --- ѕараметры исходного эксперимента ---
		% либо передаютс€ аргументом, либо задаютс€ непосредственно тут
		parameters_struct = struct;
		% ---- CUT! ----
		
		% --- ќсновные параметры генетического поиска ---
		% ѕеречисление параметров, которые будут участвовать в генетическом
		% поиске. ‘ормат задани€ параметров следующий:
		% 'им€_параметра'	[мин макс величина_мутации веро€тность_мутации]
		parameters_to_find = {
			% ---- CUT! ----
		};
		
		% размер попул€ции
		population_size = 200;
		% максимальное количество эволюций попул€ции вцелом
		max_iterations = 10;
		% количество скрещиваний на каждом шаге эволюции
		max_combinations = 100;
		% число счастливчиков, попадающих в новую попул€цию не по причине своей
		% приспособленности, а случайным образом. “аким образом,
		% рассматриваетс€ возможность привнесени€ случайных генов от попул€ции
		% к попул€ции
		lucky_amount = round(population_size * 0.05);
		% "сила катаклизма" - количество индивидуумов умирающих при катаклизме
		cataclysm_power = round(population_size * 0.5);
		% максимальное число катаклизмов за весь период эволюции
		max_cataslysms = 5;
		% считать ли количество найденных во врем€ эволюции (замедл€ет процесс)
		% но делает возможным поставить целью нахождение определенного
		% количества профилей
		realtime_count = false;
		% максимальное количество профилей, которое требуетс€ найти
		max_individuals = 1000;
		
		 % --- ќпределение целей поиска ---
		ps = ones(size(parameters_struct.f)) * 1e-12;
		% ---- CUT! ----
		
		% --- ѕараметры финального отображени€ графиков ---
		show_selected_individuals = true;
% 	    show_selected_individuals = false;
% 		show_final_profiles = true;
		show_final_profiles = false;
		show_evolution_history = true;
% 	    show_evolution_history = false;
	end
	
	% --- ќпределение целей поиска ---
	% ---- CUT! ----
    
    % ---------------- additional functions ----------------

	function individual = set_parameter(individual, parameter, value)
		eval(['individual.', parameter, '=', num2str(value), ';']);
	end

	function value = get_parameter(individual, parameter)
		value = 0;
		eval(['value = individual.', parameter, ';']);
	end
	
    function tau = calculate_tau(individual)
        direct_task_solution = direct_task(individual);
        tau = real(direct_task_solution.tau)';
    end
    
    function [fittness, individual] = fittness_function(individual)
        tau = calculate_tau(individual);
        fittness = sum(abs(tau - min_tau) + abs(max_tau - tau) - dtau);
        individual.fittness = fittness;
        individual.tau = tau';
	end

	function individual = generate_individual()
		individual = parameters_struct;
		
		for ii = 1:size(parameters_to_find, 1)
			a = parameters_to_find{ii, 2}(1);
			b = parameters_to_find{ii, 2}(2);
			parameter_value = rand() * (b - a) + a;
			individual = set_parameter(individual, parameters_to_find{ii, 1}, parameter_value);
		end
		individual.fittness = Inf;
		individual.tau = [];
	end
    
    function population = generate_population()
       population = generate_individual();
       for ii = 1:population_size           
           population(ii) = generate_individual();
       end
    end

    function new_population = killing(population)
        res_array = zeros(size(population, 2), 2);
        for ii = 1:size(population, 2)
            individual_fittness = population(ii).fittness;
            if individual_fittness == Inf
                [individual_fittness, population(ii)] = fittness_function(population(ii));
            end
            res_array(ii, :) = [ii individual_fittness];
        end
        res_array = sortrows(res_array, 2);
%         res_array = res_array(randperm(size(res_array, 1)), :);
        
        new_population = population(1);
        for ii = 1:population_size - lucky_amount
            new_population(ii) = population(res_array(ii, 1));
        end
        
        random_lucky = randi([population_size - lucky_amount, population_size + max_combinations], [1 lucky_amount]);
        for ii = 1:lucky_amount
            new_population(population_size - lucky_amount + ii) = population(res_array(random_lucky(ii), 1));
        end
    end

    function individual = crossover(individual_1, individual_2)
        crossover_point = randi([1, size(parameters_to_find, 1) - 1]);
        
        individual = individual_1;
        individual.fittness = Inf;
        for ii = 1:size(parameters_to_find, 1)
            if ii > crossover_point
				param = parameters_to_find{ii, 1};
				individual = set_parameter(individual, param, get_parameter(individual_2, param));
            end
        end
    end

    function individual = mutate(individual)
        individual.fittness = Inf;
        for ii = 1:size(parameters_to_find, 1)
			mutation_probability = parameters_to_find{ii, 2}(4);
            if rand() < mutation_probability
				old_value = get_parameter(individual, parameters_to_find{ii, 1});
				new_value = old_value + (rand() - 0.5) * 2 * parameters_to_find{ii, 2}(3);
                individual = set_parameter(individual, parameters_to_find{ii, 1}, new_value);
            end
        end
    end

    function population = do_the_cataclysm(population)
        unlucky = randperm(population_size, cataclysm_power);
		
		for ii = 1:cataclysm_power
			[fittness, population(unlucky(ii))] = fittness_function(generate_individual());
		end
		disp('Cataclysm!');
    end

    function answer = are_individuals_same(individual_1, individual_2)
        answer = true;
        for ii = 1:size(parameters_to_find, 1)
            param = parameters_to_find{ii, 1};
            if get_parameter(individual_1, param) ~= get_parameter(individual_2, param)
                answer = false;
                break;
            end
        end
    end

    function answer = are_populations_same(population_1, population_2)
        answer = true;
        for ii = 1:size(population_1, 2)
            if ~are_individuals_same(population_1(ii), population_2(ii))
                answer = false;
                break;
            end
        end
    end

    function uniqums = select_unique(individuals)
        uniqums = individuals(1);
        
        for ii = 2:size(individuals, 2)
            is_unique = true;
            for jj = 1:size(uniqums, 2)
                if are_individuals_same(individuals(ii), uniqums(jj))
                    is_unique = false;
                    break;
                end
            end
            if is_unique
                uniqums(size(uniqums, 2) + 1) = individuals(ii);
            end
        end
    end

	% ---- CUT! ----

    % ---------------- main algorhithm ----------------

    population = generate_population();
    
    resulting_individuals = struct();
    evolution_history = struct();
    evolution_history.population = struct();
    
    step = 1;
    cataclysms = 0;
    was_cataclysm = false;
    evolution_break = false;
	
	disp('Starting evolution...');
    
    while step <= max_iterations
        evolution_history(step).population = population;
        evolution_history(step).std = std([population.fittness]);
        
        combinations = combnk(1:population_size, 2);
        combinations = combinations(randperm(size(combinations, 1)), :);
        combinations = combinations(1:max_combinations, :);
        
        new_population = population;
        
        for i = 1:max_combinations
            individual_1 = mutate(population(combinations(i, 1)));
            individual_2 = mutate(population(combinations(i, 2)));
            
            new_individual = crossover(individual_1, individual_2);
            new_population(population_size + i) = new_individual;
        end
        
        new_population = killing(new_population);
        
        if are_populations_same(new_population, evolution_history(step).population)
            if was_cataclysm || cataclysms >= max_cataslysms
                evolution_break = true;
            else
                new_population = do_the_cataclysm(new_population);
                cataclysms = cataclysms + 1;
                was_cataclysm = true;
            end
        else
            was_cataclysm = false;
        end
        
        population = new_population;
        
        for i = 1:population_size
			if population(i).fittness < min(dtau) / 1000
                saved_inidividuals = size(resulting_individuals, 2);
                if isempty(fieldnames(resulting_individuals))
                    resulting_individuals = population(i);
                else
                    resulting_individuals(saved_inidividuals + 1) = population(i);
                end
			end
        end
		
        if realtime_count
            resulting_individuals = select_unique(resulting_individuals);
            fprintf('Step: %d \tFound: %d \tSTD: %d \tMAX: %d\n', step, size(resulting_individuals, 2), std([population.fittness]), max([population.fittness]));
        else
            fprintf('Step: %d \tSTD: %d \tMAX: %d\n', step, std([population.fittness]), max([population.fittness]));
        end
        
        step = step + 1;
		
		if realtime_count && size(resulting_individuals, 2) >= max_individuals
			disp('Requred amount of profiles found. Stop.');
			break;
		end
        
        if evolution_break
            disp('Evolution stopped.')
            break;
        end
    end
    
    if ~realtime_count
        disp('Removing dublicates...');
        resulting_individuals = select_unique(resulting_individuals);
        disp('Done.');
    end
	
	evolution_history(step).population = population;
    
    fprintf('Total found: %d\n',size(resulting_individuals, 2));
	
    % visualizations
	if size(resulting_individuals, 2) > 1
        % ---- CUT! ----
	end
    
    if show_evolution_history
        % ---- CUT! ----
    end
end
