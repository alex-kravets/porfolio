function res = exp_view(mat_file)

	if ischar(mat_file)
		persistent ExpStruct;
		datas = load(mat_file);
		ExpStruct = datas.ExpStruct;
	else
		if isstruct(mat_file)
			global ExpStruct;
			ExpStruct = mat_file;
		else
			disp('Переданный объект не является структурой.');
			return;
		end
	end

	main_margins	= [10 10];
	div_margins		= [0 15];
	bot_margin		= 25;
	gr_margins		= [30 60];
	info_margins	= [20 20];
	graphs			= [2 1];
	all_grids		= 'on';

	min_size		= [200 100];
	tof_marks		= {'ro', 'kh', 'b*', 'go', 'yo', 'co'};
	det_colors		= {'b', 'r', 'c', 'k', 'g'};

	% *****************************

	main_fig = figure('MenuBar', 'none', ...
		'NumberTitle', 'off', ...
		'DeleteFcn', @closeFcn);

	valid = zeros(graphs);
	index = 1;
	gr_per_page = graphs(1) * graphs(2);
	pages_num = ceil((size(ExpStruct, 2) - 1) / gr_per_page);
	sub_wins = struct([]);

	up = cell(graphs(2), graphs(1));
	gr = cell(graphs(2), graphs(1));
	pb = cell(graphs(2), graphs(1));
	pbm = cell(graphs(2), graphs(1));
	txt = cell(graphs(2), graphs(1));
	imp = cell(graphs(2), graphs(1));
	tb = cell(graphs(2), graphs(1));

	for i = 1:graphs(1)
		for j = 1:graphs(2)
			up{j, i} = uipanel(...
			'Units', 'pixels', ...
			'BorderType', 'none', ...
			'Title', '');
		
			gr{j, i} = axes('Parent', up{j, i}, ...
				'Units', 'pixels');
			hold(gr{j, i}, 'on');
			
			pb{j, i} = uicontrol('Style', 'PushButton', ...
				'Parent', up{j, i}, ...
				'String', 'Исправить', ...
				'Position', [gr_margins(1) 5 70 20], ...
				'Callback', {@changePressed [i j]});
			
			tb{j, i} = uicontrol('Style', 'ToggleButton', ...
				'Parent', up{j, i}, ...
				'String', 'Верно', ...
				'Position', [gr_margins(1) + 80 5 60 20], ...
				'Callback', {@OKPressed [i j]});
			
			pbm{j, i} = uicontrol('Style', 'PushButton', ...
				'Parent', up{j, i}, ...
				'String', 'Подробнее', ...
				'Position', [gr_margins(1) + 150 5 70 20], ...
				'Callback', {@morePressed [i j]});
			
			imp{j, i} = uicontrol('Style', 'CheckBox', ...
				'Parent', up{j, i}, ...
				'String', 'Важный', ...
				'Position', [gr_margins(1) + 230 5 70 20], ...
				'Callback', {@importantPressed [i j]});
			
			txt{j, i} = annotation(up{j, i}, 'textbox');
			txt{j, i}.Units = 'pixels';
			txt{j, i}.Position = [gr_margins(1) + 300 5 100 20];
			txt{j, i}.String = 'SqDev';
			txt{j, i}.FontSize = 8;
			txt{j, i}.EdgeColor = 'none';
		end
	end

	pb_quit = uicontrol('Style', 'PushButton', ...
			'Parent', main_fig, ...
			'String', 'Выход', ...
			'Callback', 'close'); 

	pb_next = uicontrol('Style', 'PushButton', ...
			'Parent', main_fig, ...
			'String', 'Далее ->', ...
			'Callback', @nextPressed);  
		
	pb_prev = uicontrol('Style', 'PushButton', ...
			'Parent', main_fig, ...
			'String', '<- Назад', ...
			'Enable', 'off', ...
			'Callback', @prevPressed); 
		
	pb_save = uicontrol('Style', 'PushButton', ...
			'Parent', main_fig, ...
			'String', 'Сохранить', ...
			'Callback', @savePressed);
		
	tb_mode = uicontrol('Style', 'ToggleButton', ...
			'Parent', main_fig, ...
			'String', 'Просмотр', ...
			'Value', 1, ...
			'Callback', @modePressed);
		
	tb_auto_view = uicontrol('Style', 'ToggleButton', ...
			'Parent', main_fig, ...
			'String', 'Автоматически', ...
			'Callback', @autoviewPressed);
			
	main_fig.SizeChangedFcn = @resizeCallback;
	main_fig.WindowKeyPressFcn = @keys;
	load_page();
	resizeCallback(main_fig, []);
	modePressed(tb_mode, []);

%	Callbacks

	function keys(h, eventdata)		
		control = 0;
		shift = 0;
		alt = 0;
		
		for ii = 1:size(eventdata.Modifier, 2)
			if strcmp(eventdata.Modifier{ii}, 'control')
				control = 1;
			end
			if strcmp(eventdata.Modifier{ii}, 'shift')
				shift = 1;
			end
			if strcmp(eventdata.Modifier{ii}, 'alt')
				alt = 1;
			end
		end
		
		if control && strcmp(eventdata.Key, 'g')
			if get(tb_mode, 'Value') == 1
				go_to_page();
			end
		end
		
		if control && strcmp(eventdata.Key, 'e')
			if get(tb_mode, 'Value') == 1
				go_to_exp();
			end
		end
		
		if control && strcmp(eventdata.Key, 'd')
			show_detect();
		end
		
		if control && strcmp(eventdata.Key, 't')
			show_tofs();
		end
		
		if control && strcmp(eventdata.Key, 's')
			savePressed(0, 0);
		end
		
		if strcmp(eventdata.Key, 'leftarrow')
			prevPressed(0, 0);
		end
		
		if strcmp(eventdata.Key, 'rightarrow')
			nextPressed(0, 0);
		end
		
	end

    function resizeCallback(h, eventdata)
        pos = h.Position;
        gr_width = (pos(3) - main_margins(1) * 2 - div_margins(1) * (graphs(1) - 1)) / graphs(1);
        gr_height = (pos(4) - main_margins(2) * 2 - div_margins(2) * (graphs(2) - 1) - bot_margin) / graphs(2);
        new_size = floor([gr_width, gr_height]);
        if sum(new_size < min_size)
            new_size = min_size;
        end
        
        for li = 1:graphs(1)
            new_pos(1) = main_margins(1) + (li - 1) * (new_size(1) + div_margins(1));
            new_pos(2) = pos(4) - main_margins(2) - gr_height;
            for lj = 1:graphs(2)
                set(up{lj, li}, 'Position', [new_pos new_size]);
                new_pos(2) = new_pos(2) - (gr_height + div_margins(2));
                set(gr{lj, li}, 'Position', [gr_margins (new_size - gr_margins - [15 15])]);
            end 
        end
        set(tb_mode, 'Position', [main_fig.Position(3)/2 - 235 10 70 20]); 
        set(pb_prev, 'Position', [main_fig.Position(3)/2 - 155 10 60 20]);
        set(pb_save, 'Position', [main_fig.Position(3)/2 - 85 10 80 20]);  
        set(pb_quit, 'Position', [main_fig.Position(3)/2 + 5 10 60 20]); 
        set(pb_next, 'Position', [main_fig.Position(3)/2 + 75 10 60 20]); 
        set(tb_auto_view, 'Position', [main_fig.Position(3)/2 + 145 10 100 20]);
    end

    function OKPressed(h, eventdata, crd)
        li = crd(1); lj = crd(2);
        valid(li, lj) = get(h, 'Value');
        if valid(li, lj)
            set(pb{lj, li}, 'Enable', 'off');
        else
            set(pb{lj, li}, 'Enable', 'on');
            changePressed(pb{lj, li}, [], crd);
        end
    end

    function changePressed(h, eventdata, crd)
        li = crd(1); lj = crd(2);
        idx = get_index(li, lj);
        
        [x, y] = ginput(1);        
        ids = find(ExpStruct(get_index(li, lj)).detect(ExpStruct(idx).signal_div.detect_from:ExpStruct(idx).signal_div.detect_to, 1) > x);
        
        ExpStruct(idx).tof_id.user = ids(1) + ExpStruct(idx).signal_div.detect_from - 1;
        ExpStruct(idx).tof_time.user = ExpStruct(idx).detect(ExpStruct(idx).tof_id.user, 1);
        gr_load(li, lj);
        
        set(tb{lj, li}, 'Value', 1);
        OKPressed(tb{lj, li}, [], crd);
    end

    function nextPressed(h, eventdata)
        if get(tb_mode, 'Value') == 1
            index = index + 1;
            load_page();
            return;
        end
        if sum(sum(valid)) == gr_per_page            
            valid = zeros(graphs);
            
            for li = 1:graphs(1)
                for lj = 1:graphs(2)
                    set(pb{lj, li}, 'Enable', 'on');
                    set(tb{lj, li}, 'Value', 0);
                    
                    idx = get_index(li, lj);
                    if idx <= size(ExpStruct, 2)
                        ExpStruct(idx).tof_valid = 1;
                    end
                end
            end
            index = index + 1;
            load_page();
        else
            % иначе ничего не делать
        end
    end

    function prevPressed(h, eventdata)
        index = index - 1;
        load_page();
    end

    function savePressed(h, eventdata)
        for li = 1:graphs(1)
            for lj = 1:graphs(2)
                idx = get_index(li, lj);
                if idx <= size(ExpStruct, 2)
                    ExpStruct(idx).tof_valid = valid(li, lj);
                end
            end
        end
		if ischar(mat_file)
			save(mat_file, 'ExpStruct');
		end
    end

    function morePressed(h, eventdata, crd)
        li = crd(1); lj = crd(2);
        idx = get_index(li, lj);
        ind = size(sub_wins, 2) + 1;
        
        set(pbm{lj, li}, 'Enable', 'off');
 
        params = ExpStruct(idx).parameters;
        comment = ExpStruct(idx).comment;
       
        cmt_h = 50;
        fnames = fieldnames(params);
        data = cell(size(fnames, 1), 1);
        for lli = 1:size(fnames, 1)
            data{lli} = params.(fnames{lli});
        end
        
        sub_wins(ind).win = figure('MenuBar', 'none', ...
            'Name', ['Информация об эксперименте № ', num2str(idx-1)], ...
            'NumberTitle', 'off', ...
            'DeleteFcn', @delInfo);
        sub_wins(ind).idx = idx;
        
        mtb = uitable('Parent', sub_wins(ind).win, ...
            'ColumnFormat', {'char', 'char'}, ...
            'ColumnName', {'Параметр', 'Значение'}, ...
            'Data', [fnames data], ...
            'TooltipString', 'Параметры эксперимента');
        
        cmt = uicontrol('Parent', sub_wins(ind).win, ...
            'Style', 'Edit', ...
            'Max', 2, ...
            'Callback', @cmtSave, ...
            'String', comment, ...
            'TooltipString', 'Комментарий к эксперименту');
        
        sub_wins(ind).win.SizeChangedFcn = @infoResized;
        infoResized(sub_wins(ind).win, []);
        
        function infoResized(h, eventdata)
            pos = get(h, 'Position');
            new_size = [pos(3) pos(4)] - 2 * info_margins - [0 cmt_h + 5];
            set(mtb, 'Position', [info_margins + [0 cmt_h + 5] new_size]);
            set(cmt, 'Position', [info_margins [new_size(1) cmt_h]]);
            set(mtb, 'ColumnWidth', {200, 200});
        end
        
        function delInfo(h, eventdata)
            sub_wins(ind).win = 0;
            if get_index(li, lj) == sub_wins(ind).idx
                set(pbm{lj, li}, 'Enable', 'on');
            end
            
            last_opened = 1;
            for llli = 1:size(sub_wins, 2)
                if sub_wins(llli).win ~= 0
                    last_opened = 0;
                end
            end
            if last_opened
                sub_wins = struct([]);
            end
            cmtSave(h, []);
        end
        
        function cmtSave(h, eventdata)
            ExpStruct(idx).comment = get(cmt, 'String');
        end
    end

    function importantPressed(h, eventdata, crd)
        li = crd(1); lj = crd(2);
        important = get(h, 'Value');
        ExpStruct(get_index(li, lj)).important = important;
        gr_load(li, lj);
    end

	function modePressed(h, eventdata)
        if get(h, 'Value') == 1
            set(tb_auto_view, 'Visible', 'on');
            set(h, 'String', 'Просмотр');
            for li = 1:graphs(1)
                for lj = 1:graphs(2)
                    set(pb{lj, li}, 'Enable', 'off');
                    set(tb{lj, li}, 'Enable', 'off');
                end
            end
        else
            set(tb_auto_view, 'Visible', 'off');
            set(h, 'String', 'Изменение');
            for li = 1:graphs(1)
                for lj = 1:graphs(2)
                    set(pb{lj, li}, 'Enable', 'on');
                    set(tb{lj, li}, 'Enable', 'on');
                end
            end
        end
    end

    function closeFcn(h, eventdata)
        savePressed(h, eventdata);
        for li = 1:size(sub_wins, 2)
            if sub_wins(li).win ~= 0
                close(sub_wins(li).win);
            end
        end
	end

    function autoviewPressed(h, eventdata)
        auto = get(h, 'Value');
        if ~strcmp(get(pb_next, 'Enable'), 'off')
            autoview();
        end
	end

%	Additional

	function update_name
		main_fig.Name = ['Просмотр экспериментов (', num2str(size(ExpStruct, 2)), ') | ', num2str(index), '/', num2str(pages_num)];
	end

	function load_page()
		gr_load_all();
		update_name();
		
		if index == 1
			set(pb_prev, 'Enable', 'off');
		else
			set(pb_prev, 'Enable', 'on');
		end
		
		if index == pages_num
			set(pb_next, 'Enable', 'off');
		else
			set(pb_next, 'Enable', 'on');
		end
	end

    function gr_load(li, lj)
		idx = get_index(li, lj);
        
		% ---- CUT! ----
		
	end

    function gr_load_all()
        for li = 1:graphs(1)
            for lj = 1:graphs(2)
                gr_load(li, lj);
            end
        end
    end

    function idx = get_index(li, lj)
        idx = (index - 1) * gr_per_page + (li - 1) * graphs(2) + lj + 1;
    end

    function autoview()
        auto = get(tb_auto_view, 'Value');
        if auto
            nextPressed(pb_next, []);
            pause(2);
            if ~strcmp(get(pb_next, 'Enable'), 'off')
                autoview();
            end
        end
	end

	function go_to_page()
		m_size = main_fig.Position;
		p_size = [250 80];
		p_pos = p_size;
		p_pos(1) = m_size(1) + ceil((m_size(3) - p_size(1)) / 2);
		p_pos(2) = m_size(2) + ceil((m_size(4) - p_size(2)) / 2);

		page_selection = figure('MenuBar', 'none', ...
			'NumberTitle', 'off', ...
			'WindowStyle', 'modal', ...
			'Position', [p_pos p_size], ...
			'Name', 'Перейти к странице', ...
			'Resize', 'off');

		page_text = uicontrol('Style', 'Edit', ...
			'Parent', page_selection, ...
			'Position', [20 50 210 20], ...
			'Callback', @page_pressed);

		page_button = uicontrol('Style', 'PushButton', ...
			'Parent', page_selection, ...
			'String', 'Перейти', ...
			'Position', [95 15 60 20], ...
			'Callback', @page_pressed);

		function page_pressed(h, eventdata)
			pg = str2num(page_text.String);
			if size(pg, 1)
				page_to = pg;
			else
				page_to = index;
			end
			
			if page_to < 1
				page_to = index;
			end
			
			if page_to > pages_num
				page_to = index;
			end
			
			if page_to ~= index
				index = page_to;
				load_page();
				close(page_selection);
			end
		end
	end

	function go_to_exp()
		m_size = main_fig.Position;
		p_size = [250 80];
		p_pos = p_size;
		p_pos(1) = m_size(1) + ceil((m_size(3) - p_size(1)) / 2);
		p_pos(2) = m_size(2) + ceil((m_size(4) - p_size(2)) / 2);

		exp_selection = figure('MenuBar', 'none', ...
			'NumberTitle', 'off', ...
			'WindowStyle', 'modal', ...
			'Position', [p_pos p_size], ...
			'Name', 'Перейти к эксперименту', ...
			'Resize', 'off');

		exp_text = uicontrol('Style', 'Edit', ...
			'Parent', exp_selection, ...
			'Position', [20 50 210 20], ...
			'Callback', @exp_pressed);

		exp_button = uicontrol('Style', 'PushButton', ...
			'Parent', exp_selection, ...
			'String', 'Перейти', ...
			'Position', [95 15 60 20], ...
			'Callback', @exp_pressed);

		function exp_pressed(h, eventdata)
			try_again_exp = 0;
			total_exp = size(ExpStruct, 2) - 1;
			
			ex = str2num(exp_text.String);
			if size(ex, 1)
				exp_to = ex;
			else
				exp_to = 0;
			end
			
			if exp_to < 1
				try_again_exp = 1;
			end
			
			if exp_to > total_exp
				try_again_exp = 1;
			end
			
			if try_again_exp
				exp_text.String = '';
			end

			close(exp_selection);
			page_num = ceil(exp_to / gr_per_page);
			index = page_num;
			load_page();
		end
	end

	function show_tofs()
		m_size = main_fig.Position;
		p_size = [250 120];
		p_pos = p_size;
		p_pos(1) = m_size(1) + ceil((m_size(3) - p_size(1)) / 2);
		p_pos(2) = m_size(2) + ceil((m_size(4) - p_size(2)) / 2);
		
		total = size(ExpStruct, 2) - 1;
		methods = [];
		if total > 1
			methods = fieldnames(ExpStruct(2).tof_id);
		end

		tofs_selection = figure('MenuBar', 'none', ...
			'NumberTitle', 'off', ...
			'WindowStyle', 'modal', ...
			'Position', [p_pos p_size], ...
			'Name', 'Времена привязки', ...
			'Resize', 'off');

		tofs_range = uicontrol('Style', 'Edit', ...
			'Parent', tofs_selection, ...
			'String', ['1,', num2str(total)], ...
			'Position', [20 90 210 20], ...
			'Callback', @tofs_pressed);

		tofs_methods = uicontrol('Style', 'Edit', ...
			'Parent', tofs_selection, ...
			'String', strjoin(methods, ','), ...
			'Position', [20 50 210 20], ...
			'Callback', @tofs_pressed);

		tofs_button = uicontrol('Style', 'PushButton', ...
			'Parent', tofs_selection, ...
			'String', 'Показать', ...
			'Position', [95 15 60 20], ...
			'Callback', @tofs_pressed);

		function tofs_pressed(h, eventdata)
			try_again_r = 0;
			try_again_m = 0;
			
			range_txt = tofs_range.String;
			range_s = strsplit(range_txt, ',');
			
			if size(range_s, 2) > 2 || size(range_s, 2) < 1
				try_again_r = 1;
			else
				r_from = str2num(range_s{1});
				r_to = str2num(range_s{2});
				if r_from < 1 || r_to > total || r_from > r_to
					try_again_r = 1;
				end
			end
			
			if try_again_r
				tofs_range.String = ['1,', num2str(total)];
				return;
			end
			
			mthd = tofs_methods.String;
			mthd = strsplit(mthd, ',');
			
			if size(mthd, 2) == 0
				try_again_m = 1;
			else
				for ii = 1:size(mthd, 2)
					in_methods = 0;
					for jj = 1:size(methods, 1)
						if strcmp(mthd{ii}, methods{jj})
							in_methods = 1;
							break;
						end
					end
					if ~in_methods
						try_again_m = 1;
						break;
					end
				end
			end
			
			if try_again_m
				tofs_methods.String = strjoin(methods, ',');
				return;
			end
			close(tofs_selection);
			
			figure();
			all_labels = struct;
			for ii = r_from:r_to
				idx = ii + 1;
				for jj = 1:size(mthd, 2)
					if isfield(ExpStruct(idx).tof_time, mthd{jj})
						if ExpStruct(idx).tof_time.(mthd{jj}) ~= 0 && ExpStruct(idx).tof_id.(mthd{jj}) ~= size(ExpStruct(idx).detect, 1)
							all_labels.(mthd{jj}) = plot(ii, ExpStruct(idx).tof_time.(mthd{jj}), tof_marks{jj}); hold on;
						end
					end
				end
			end
			
			tofs_labels = fieldnames(all_labels);
			tofs_points = zeros(1, size(tofs_labels, 1));
			
			for ii = 1:size(tofs_labels, 1)
				tofs_points(ii) = all_labels.(tofs_labels{ii});
			end
			legend(tofs_points, tofs_labels);
			xlabel('№');
			ylabel('t');
			grid on;
			grid minor;
		end
	end

	function show_detect()
		m_size = main_fig.Position;
		p_size = [250 80];
		p_pos = p_size;
		p_pos(1) = m_size(1) + ceil((m_size(3) - p_size(1)) / 2);
		p_pos(2) = m_size(2) + ceil((m_size(4) - p_size(2)) / 2);

		first_gr = gr_per_page * (index - 1) + 1;
		
		detect_selection = figure('MenuBar', 'none', ...
			'NumberTitle', 'off', ...
			'WindowStyle', 'modal', ...
			'Position', [p_pos p_size], ...
			'Name', 'Просмотр сигнала', ...
			'Resize', 'off');

		detect_text = uicontrol('Style', 'Edit', ...
			'Parent', detect_selection, ...
			'String', num2str(first_gr), ...
			'Position', [20 50 210 20], ...
			'Callback', @detect_pressed);

		detect_button = uicontrol('Style', 'PushButton', ...
			'Parent', detect_selection, ...
			'String', 'Просмотр', ...
			'Position', [95 15 60 20], ...
			'Callback', @detect_pressed);

		function detect_pressed(h, eventdata)
			detect_signals = detect_text.String;
			close(detect_selection);
			if strcmp(detect_signals, 'a')
				to_show = ones(1, gr_per_page) * first_gr;
				for ii = 2:gr_per_page
					to_show(ii) = to_show(ii - 1) + 1;
				end
			else
				signals = strsplit(detect_signals, ',');
				to_show = [];
				for ii = 1:size(signals, 2)
					dn = str2num(signals{ii});
					if size(dn, 1)
						if abs(dn) >= 1 && abs(dn) <= size(ExpStruct, 2)
							to_show(size(to_show, 2) + 1) = dn;
						end
					end
				end
			end
			
			if size(to_show, 2) == 0
				return;
			end
			
			figure();
			gr_marks = {};
			sg_lines = zeros(1, size(to_show, 2));
			all_tof_labels = struct;
			
			for ii = 1:size(to_show, 2)
				idx = abs(to_show(ii)) + 1;
				sg_lines(ii) = plot(ExpStruct(idx).detect(:, 1), ExpStruct(idx).detect(:, 2), [det_colors{ii}, '-']); hold on;
				gr_marks{size(gr_marks, 2) + 1} = ['Эксп. № ', num2str(idx - 1)];
			end
			
			for ii = 1:size(to_show, 2)
				if to_show(ii) < 0
					continue;
				end
				idx = abs(to_show(ii)) + 1;
				plot(ExpStruct(idx).detect(ExpStruct(idx).signal_div.source_from, 1) * [1 1], ExpStruct(idx).signal_div.source_max * [0 1], [det_colors{ii}, ':']);
				plot(ExpStruct(idx).detect(ExpStruct(idx).signal_div.source_to, 1) * [1 1], ExpStruct(idx).signal_div.source_max * [0 1], [det_colors{ii}, ':']);
				plot(ExpStruct(idx).detect(ExpStruct(idx).signal_div.detect_from, 1) * [1 1], ExpStruct(idx).signal_div.detect_max * [0 1], [det_colors{ii}, '--']);
				plot(ExpStruct(idx).detect(ExpStruct(idx).signal_div.detect_to, 1) * [1 1], ExpStruct(idx).signal_div.detect_max * [0 1], [det_colors{ii}, '--']);
				
				tofs = fieldnames(ExpStruct(idx).tof_id);
				for jj = 1:size(tofs, 1)
					t_id = ExpStruct(idx).tof_id.(tofs{jj});
					if t_id ~= 0
						all_tof_labels.(tofs{jj}) = plot(ExpStruct(idx).detect(t_id, 1), ExpStruct(idx).detect(t_id, 2), tof_marks{jj});
					end
				end
			end
			
			tof_labels = fieldnames(all_tof_labels);
			for ii = 1:size(tof_labels, 1)
				sg_lines(size(sg_lines, 2) + 1) = all_tof_labels.(tof_labels{ii});
				gr_marks{size(gr_marks, 2) + 1} = tof_labels{ii};
			end
			legend(sg_lines, gr_marks);
			xlabel('t');
			ylabel('A');
			grid on;
			grid minor;
		end
	end

    res = ExpStruct;
end